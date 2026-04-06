#include <proc/task.h>
#include <mm/paging.h>
#include <mm/kheap.h>
#include <string.h>
#include <vga.h>
#include <serial.h>
#include <interrupts.h>

static void idle_task_fn(void) { while(1) { __asm__("hlt"); } }
static void init_task_fn(void) { while(1) { for(int i=0;i<1000000;i++) __asm__("nop"); } }

// 全局变量
task_t* ready_queue[MAX_PRIORITY];  // 优先级就绪队列
task_t* current_task = NULL;        // 当前运行进程
static uint32_t next_pid = 1;              // 下一个PID
static uint32_t system_ticks = 0;          // 系统时钟中断计数

// 时间片大小（时钟中断次数）
#define TIME_SLICE 10

// 获取系统时钟中断次数
uint32_t get_system_ticks(void)
{
    return system_ticks;
}

// 辅助函数：获取对齐的内存地址
static void* align_address(void* addr, uint32_t align)
{
    return (void*)((uint32_t)addr & ~(align - 1));
}

// 创建空闲进程
static task_t* create_idle_task(void)
{
    // 分配PCB
    task_t* idle_task = (task_t*)kmalloc(sizeof(task_t));
    if (!idle_task) {
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    memset(idle_task, 0, sizeof(task_t));
    idle_task->pid = 0;
    idle_task->state = TASK_READY;
    idle_task->priority = 0;  // 最低优先级
    idle_task->page_dir = get_kernel_page_dir();
    idle_task->time_slice = TIME_SLICE;
    idle_task->total_runtime = 0;
    idle_task->last_scheduled = 0;
    idle_task->memory_usage_kb = 4;
    strcpy(idle_task->name, "idle");
    
    // 分配内核栈
    void* kernel_stack = kmalloc(4096);
    if (!kernel_stack) {
        kfree(idle_task);
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    idle_task->kernel_stack_top = (uint32_t)kernel_stack + 4096;
    
    // 设置初始上下文（idle任务的入口点）
    idle_task->regs.eip = (uint32_t)idle_task_fn;

    idle_task->regs.eflags = 0x202;  // IF=1
    idle_task->regs.cs = 0x08;       // 内核代码段
    idle_task->regs.ds = 0x10;       // 内核数据段
    idle_task->regs.es = 0x10;
    idle_task->regs.fs = 0x10;
    idle_task->regs.gs = 0x10;
    idle_task->regs.ss = 0x10;
    idle_task->regs.esp = idle_task->kernel_stack_top;
    
    return idle_task;
}

// 初始化调度器
void sched_init(void)
{
    serial_write_string("[LOG]\n");
    
    // 初始化就绪队列
    for (int i = 0; i < MAX_PRIORITY; i++) {
        ready_queue[i] = NULL;
    }
    
    // 创建空闲进程（PID 0）
    task_t* idle_task = create_idle_task();
    if (!idle_task) {
        serial_write_string("[LOG]\n");
        return;
    }
    
    // 将idle任务加入就绪队列
    ready_queue[0] = idle_task;
    
    // 创建init进程（PID 1）
    task_t* init_task = create_task(
        (void*)init_task_fn,
        "init",
        5
    );
    
    if (!init_task) {
        serial_write_string("[LOG]\n");
        return;
    }
    
    // 将init任务加入就绪队列
    ready_queue[5] = init_task;
    
    // 设置当前进程为idle任务
    current_task = idle_task;
    
    serial_write_string("[LOG]\n");
}

// 创建新进程
task_t* create_task(void (*entry)(void), const char* name, uint32_t priority)
{
    if (priority >= MAX_PRIORITY) {
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    // 分配PCB
    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    if (!task) {
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    memset(task, 0, sizeof(task_t));
    task->pid = next_pid++;
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = TIME_SLICE;
    task->total_runtime = 0;
    task->last_scheduled = 0;
    task->memory_usage_kb = 8;
    strcpy(task->name, name);
    
    // 复制当前进程的页目录（或创建新的）
    task->page_dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!task->page_dir) {
        kfree(task);
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    // 复制内核页目录映射
    memcpy(task->page_dir, get_kernel_page_dir(), sizeof(page_directory_t));
    
    // 分配内核栈
    void* kernel_stack = kmalloc(4096);
    if (!kernel_stack) {
        kfree(task->page_dir);
        kfree(task);
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    task->kernel_stack_top = (uint32_t)kernel_stack + 4096;
    
    // 设置初始上下文
    task->regs.eip = (uint32_t)entry;
    task->regs.eflags = 0x202;  // IF=1
    task->regs.cs = 0x08;       // 内核代码段
    task->regs.ds = 0x10;       // 内核数据段
    task->regs.es = 0x10;
    task->regs.fs = 0x10;
    task->regs.gs = 0x10;
    task->regs.ss = 0x10;
    task->regs.esp = task->kernel_stack_top;
    
    // 设置父进程
    task->parent = current_task;
    
    // 将任务加入就绪队列
    task->sibling_next = ready_queue[priority];
    ready_queue[priority] = task;
    
    serial_write_string("[LOG]\n");
    
    return task;
}

// 进程调度算法
void schedule(void)
{
    if (!current_task) {
        return;
    }
    
    // 更新当前进程的运行时间
    current_task->total_runtime++;
    current_task->time_slice--;
    
    // 如果时间片用完，将进程放回就绪队列
    if (current_task->time_slice <= 0) {
        current_task->state = TASK_READY;
        current_task->time_slice = TIME_SLICE;
        
        // 将当前进程加入就绪队列
        task_t* task = current_task;
        task->sibling_next = ready_queue[task->priority];
        ready_queue[task->priority] = task;
        
        current_task = NULL;
    }
    
    // 寻找下一个可运行的进程
    task_t* next_task = NULL;
    
    // 从高优先级到低优先级扫描就绪队列
    for (int i = MAX_PRIORITY - 1; i >= 0; i--) {
        if (ready_queue[i]) {
            // 获取队列中的第一个任务
            next_task = ready_queue[i];
            
            // 更新就绪队列
            ready_queue[i] = next_task->sibling_next;
            next_task->sibling_next = NULL;
            
            break;
        }
    }
    
    // 如果没有找到可运行的进程，使用idle任务
    if (!next_task) {
        next_task = create_idle_task();
        if (!next_task) {
            serial_write_string("[LOG]\n");
            return;
        }
    }
    
    // 更新任务状态
    next_task->state = TASK_RUNNING;
    next_task->last_scheduled = 0;
    
    // 切换到新进程
    if (current_task != next_task) {
        task_t* old_task = current_task;
        current_task = next_task;
        
        serial_write_string("[LOG]\n");
        
        // 调用上下文切换函数
        switch_process(next_task);
    }
}

// 获取当前进程
task_t* get_current_task(void)
{
    return current_task;
}

// 退出当前进程
void task_exit(int status)
{
    serial_write_string("[LOG]\n");
    
    // 设置进程状态为僵尸
    current_task->state = TASK_ZOMBIE;
    
    // 释放资源
    if (current_task->page_dir && current_task->page_dir != get_kernel_page_dir()) {
        kfree(current_task->page_dir);
    }
    
    // 调度新进程
    schedule();
}

// 时钟中断处理函数（进程调度入口）
void timer_interrupt_handler(registers_t* regs)
{
    // 递增系统时钟计数
    system_ticks++;
    
    // 暂时禁用寄存器保存（regs 参数未正确传递）
    (void)regs;
    
    // 暂时禁用调度（单进程模式，避免 switch_process 崩溃）
    // schedule();
}
