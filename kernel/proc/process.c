#include <proc.h>
#include <mm/paging.h>
#include <string.h>
#include <vga.h>
#include <serial.h>
#include <interrupts.h>

// 全局变量
static pcb_t* current_process = NULL;
static pcb_t* process_list = NULL;
static uint32_t next_pid = 1;
static scheduler_type_t current_scheduler = SCHEDULER_ROUND_ROBIN;

// 进程链表管理
static void add_process_to_list(pcb_t* process) {
    if (!process_list) {
        process_list = process;
        process->next = process;
        process->prev = process;
    } else {
        process->next = process_list;
        process->prev = process_list->prev;
        process_list->prev->next = process;
        process_list->prev = process;
    }
}

static void remove_process_from_list(pcb_t* process) {
    if (process->next == process) {
        process_list = NULL;
    } else {
        process->prev->next = process->next;
        process->next->prev = process->prev;
        if (process_list == process) {
            process_list = process->next;
        }
    }
}

// 初始化进程管理
void init_process_management(void) {
    // 创建内核进程
    pcb_t* kernel_process = (pcb_t*)kmalloc(sizeof(pcb_t));
    if (!kernel_process) {
        kprintf("[ERROR] Failed to allocate kernel process!");
        return;
    }
    
    memset(kernel_process, 0, sizeof(pcb_t));
    kernel_process->pid = 0;
    kernel_process->state = PROCESS_RUNNING;
    strcpy(kernel_process->name, "kernel");
    kernel_process->page_dir = get_kernel_page_dir();
    kernel_process->priority = 10;
    kernel_process->ticks = 0;
    
    current_process = kernel_process;
    add_process_to_list(kernel_process);
    
    kprintf("[PROCESS] Process management initialized with kernel process (PID: %d)\n", kernel_process->pid);
}

// 创建新进程
pcb_t* create_process(const char* name, void (*entry_point)(void), uint32_t priority) {
    // 分配PCB
    pcb_t* process = (pcb_t*)kmalloc(sizeof(pcb_t));
    if (!process) {
        kprintf("[ERROR] Failed to allocate PCB!");
        return NULL;
    }
    
    memset(process, 0, sizeof(pcb_t));
    process->pid = next_pid++;
    process->state = PROCESS_READY;
    strcpy(process->name, name);
    process->priority = priority;
    process->ticks = 0;
    
    // 创建进程页目录（复制内核页目录）
    process->page_dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!process->page_dir) {
        kfree(process);
        kprintf("[ERROR] Failed to allocate page directory!");
        return NULL;
    }
    
    // 复制内核页目录映射
    memcpy(process->page_dir, get_kernel_page_dir(), sizeof(page_directory_t));
    
    // 分配用户栈
    uint32_t* stack = (uint32_t*)kmalloc(8192); // 8KB栈
    if (!stack) {
        kfree(process->page_dir);
        kfree(process);
        kprintf("[ERROR] Failed to allocate stack!");
        return NULL;
    }
    
    // 设置栈指针（指向栈顶）
    process->esp = stack + 8192 / sizeof(uint32_t);
    process->ebp = process->esp;
    
    // 将进程添加到链表
    add_process_to_list(process);
    
    kprintf("[PROCESS] Created process %s (PID: %d)\n", name, process->pid);
    return process;
}

// 切换进程（汇编实现）
void switch_process(pcb_t* next_process) {
    if (!next_process || current_process == next_process) {
        return;
    }
    
    // 保存当前进程状态
    asm volatile("pusha");
    asm volatile("mov %%esp, %0" : "=r"(current_process->esp));
    asm volatile("mov %%ebp, %0" : "=r"(current_process->ebp));
    
    // 更新当前进程
    current_process = next_process;
    
    // 加载下一个进程状态
    asm volatile("mov %0, %%esp" : : "r"(next_process->esp));
    asm volatile("mov %0, %%ebp" : : "r"(next_process->ebp));
    
    // 切换页目录
    uint32_t page_dir_phys = ((uint32_t)next_process->page_dir) & 0xFFFFF000;
    asm volatile("mov %0, %%cr3" : : "r"(page_dir_phys));
    
    asm volatile("popa");
}

// 进程调度器
void schedule(void) {
    if (!process_list) {
        return;
    }
    
    pcb_t* next_process = NULL;
    
    switch (current_scheduler) {
        case SCHEDULER_ROUND_ROBIN:
            // 简单的轮转调度
            next_process = current_process->next;
            while (next_process->state != PROCESS_READY && next_process != current_process) {
                next_process = next_process->next;
            }
            break;
            
        case SCHEDULER_PRIORITY:
            // 优先级调度
            next_process = process_list;
            pcb_t* highest_priority = NULL;
            do {
                if (next_process->state == PROCESS_READY && 
                    (!highest_priority || next_process->priority < highest_priority->priority)) {
                    highest_priority = next_process;
                }
                next_process = next_process->next;
            } while (next_process != process_list);
            next_process = highest_priority;
            break;
            
        case SCHEDULER_FCFS:
            // 先来先服务
            next_process = current_process->next;
            while (next_process->state != PROCESS_READY && next_process != current_process) {
                next_process = next_process->next;
            }
            break;
    }
    
    if (next_process && next_process->state == PROCESS_READY && next_process != current_process) {
        switch_process(next_process);
    }
}

// 终止进程
void terminate_process(uint32_t pid) {
    if (pid == 0) {
        kprintf("[ERROR] Cannot terminate kernel process!");
        return;
    }
    
    pcb_t* process = process_list;
    do {
        if (process->pid == pid) {
            process->state = PROCESS_TERMINATED;
            kprintf("[PROCESS] Terminated process %s (PID: %d)\n", process->name, pid);
            
            // 释放资源
            if (process->page_dir && process->page_dir != get_kernel_page_dir()) {
                // 释放页目录和页表（简化实现）
                kfree(process->page_dir);
            }
            
            // 从链表中移除
            remove_process_from_list(process);
            kfree(process);
            
            return;
        }
        process = process->next;
    } while (process != process_list);
    
    kprintf("[ERROR] Process with PID %d not found!");
}

// 获取当前进程
pcb_t* get_current_process(void) {
    return current_process;
}

// 设置调度器类型
void set_scheduler_type(scheduler_type_t type) {
    current_scheduler = type;
    const char* scheduler_names[] = {"Round Robin", "Priority", "FCFS"};
    kprintf("[PROCESS] Set scheduler to %s\n", scheduler_names[type]);
}

// 信号处理
void send_signal(uint32_t pid, int signal) {
    // 简化实现：仅记录信号
    kprintf("[SIGNAL] Sent signal %d to process %d\n", signal, pid);
}

void handle_signal(int signal) {
    // 简化实现：仅打印信号
    kprintf("[SIGNAL] Handling signal %d\n", signal);
}

// 管道实现（简化）
int create_pipe(int pipefd[2]) {
    // 简化实现：仅返回成功
    pipefd[0] = 0;
    pipefd[1] = 1;
    return 0;
}

int pipe_write(int fd, const void* buf, size_t count) {
    // 简化实现：直接写入控制台
    kprintf("[PIPE] Write: %s\n", (const char*)buf);
    return count;
}

int pipe_read(int fd, void* buf, size_t count) {
    // 简化实现：返回0
    return 0;
}

// 内存映射实现
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    // 简化实现：分配物理内存并映射到虚拟地址
    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t virtual_page = (uint32_t)addr;
    
    if (!addr) {
        // 自动分配虚拟地址（简化：从0x80000000开始）
        virtual_page = 0x80000000;
    }
    
    for (size_t i = 0; i < pages; i++) {
        uint32_t frame = alloc_frame();
        if (frame == 0) {
            return NULL;
        }
        
        uint32_t phys_addr = frame * PAGE_SIZE;
        uint32_t virt_addr = virtual_page + i * PAGE_SIZE;
        
        // 设置页属性
        uint32_t page_flags = PAGE_PRESENT;
        if (prot & 0x2) page_flags |= PAGE_WRITABLE;
        if (prot & 0x4) page_flags |= PAGE_USER;
        
        map_page((void*)virt_addr, phys_addr, page_flags);
    }
    
    kprintf("[MMAP] Mapped %zu bytes at 0x%x\n", length, virtual_page);
    return (void*)virtual_page;
}

int munmap(void* addr, size_t length) {
    // 简化实现：取消映射并释放物理内存
    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t virt_addr = (uint32_t)addr;
    
    for (size_t i = 0; i < pages; i++) {
        uint32_t frame = get_physical_addr((void*)(virt_addr + i * PAGE_SIZE)) / PAGE_SIZE;
        if (frame != 0) {
            free_frame(frame);
        }
        unmap_page((void*)(virt_addr + i * PAGE_SIZE));
    }
    
    kprintf("[MMAP] Unmapped %zu bytes at 0x%x\n", length, virt_addr);
    return 0;
}
