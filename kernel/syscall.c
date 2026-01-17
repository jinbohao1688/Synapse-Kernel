#include <syscall.h>
#include <proc/task.h>
#include <proc/regs.h>
#include <mm/paging.h>
#include <mm/kheap.h>
#include <string.h>
#include <vga.h>
#include <serial.h>
#include <interrupts.h>
#include <keyboard.h>
#include <loader/elf.h>

// 系统调用表（外部定义，在table.S中）
extern syscall_handler_t syscall_table[];

// 系统调用处理函数的最大数量
#define MAX_SYSCALLS 12

// 系统调用入口（汇编实现，用于用户空间调用）
int syscall(int num, ...) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}

// 系统调用处理函数
void syscall_handler(struct regs* regs) {
    // 获取系统调用号
    uint32_t syscall_num = regs->eax;
    
    // 检查系统调用号是否有效
    if (syscall_num >= MAX_SYSCALLS) {
        kprintf("[ERROR] Invalid syscall number: %d\n", syscall_num);
        regs->eax = -1;
        return;
    }
    
    // 调用对应的系统调用处理函数
    syscall_handler_t handler = syscall_table[syscall_num];
    if (handler) {
        regs->eax = handler(regs);
    } else {
        kprintf("[ERROR] Syscall handler not found for %d\n", syscall_num);
        regs->eax = -1;
    }
}

// SYS_exit - 退出当前进程
int sys_exit_handler(struct regs* regs) {
    int exit_code = regs->ebx;
    task_exit(exit_code);
    return 0;
}

// SYS_fork - 创建新进程
int sys_fork_handler(struct regs* regs) {
    // 创建新进程，复制当前进程的上下文
    task_t* child = create_task(
        (void*)regs->eip, 
        "forked", 
        current_task->priority
    );
    if (!child) {
        return -1;
    }
    
    // 复制寄存器上下文（除了eax，设置为0表示子进程）
    memcpy(&child->regs, regs, sizeof(regs_context_t));
    child->regs.eax = 0; // 子进程返回0
    
    // 复制当前进程的内存管理信息
    child->page_dir = current_task->page_dir;
    child->heap_start = current_task->heap_start;
    child->heap_end = current_task->heap_end;
    child->memory_usage_kb = current_task->memory_usage_kb;
    
    // 复制用户栈顶
    child->user_stack_top = current_task->user_stack_top;
    
    // 返回子进程的PID
    return child->pid;
}

// SYS_wait - 等待子进程退出
int sys_wait_handler(struct regs* regs) {
    uint32_t pid = regs->ebx;
    
    // 简化实现：仅返回成功
    return 0;
}

// SYS_write - 写入文件
int sys_write_handler(struct regs* regs) {
    int fd = regs->ebx;
    const void* buf = (const void*)regs->ecx;
    size_t count = regs->edx;
    
    // 简化实现：仅处理标准输出（fd=1）
    if (fd == 1) {
        // 写入VGA控制台
        kwrite(buf, count);
        return count;
    }
    
    return -1;
}

// SYS_read - 读取文件
int sys_read_handler(struct regs* regs) {
    int fd = regs->ebx;
    void* buf = (void*)regs->ecx;
    size_t count = regs->edx;
    
    // 简化实现：仅处理标准输入（fd=0）
    if (fd == 0) {
        // 从键盘读取
        return keyboard_read(buf, count);
    }
    
    return -1;
}

// SYS_open - 打开文件
int sys_open_handler(struct regs* regs) {
    const char* path = (const char*)regs->ebx;
    int flags = regs->ecx;
    
    // 简化实现：返回文件描述符1（标准输出）
    return 1;
}

// SYS_close - 关闭文件
int sys_close_handler(struct regs* regs) {
    int fd = regs->ebx;
    
    // 简化实现：仅返回成功
    return 0;
}

// SYS_mmap - 内存映射
int sys_mmap_handler(struct regs* regs) {
    void* addr = (void*)regs->ebx;
    size_t length = regs->ecx;
    int prot = regs->edx;
    int flags = regs->esi;
    int fd = regs->edi;
    off_t offset = regs->ebp;
    
    // 简化实现：调用mmap函数
    void* mapped_addr = mmap(addr, length, prot, flags, fd, offset);
    if (!mapped_addr) {
        return -1;
    }
    
    return (int)mapped_addr;
}

// SYS_munmap - 取消内存映射
int sys_munmap_handler(struct regs* regs) {
    void* addr = (void*)regs->ebx;
    size_t length = regs->ecx;
    
    // 简化实现：调用munmap函数
    return munmap(addr, length);
}

// SYS_sbrk - 调整进程堆大小
int sys_sbrk_handler(struct regs* regs) {
    intptr_t increment = regs->ebx;
    uint32_t old_heap_end = current_task->heap_end;
    uint32_t new_heap_end = old_heap_end + increment;
    
    // 首次调用时初始化堆
    if (current_task->heap_start == 0) {
        // 从用户空间高端开始分配堆
        current_task->heap_start = 0x08048000 + 0x1000; // 假设程序从0x08048000开始
        current_task->heap_end = current_task->heap_start;
        old_heap_end = current_task->heap_start;
        new_heap_end = old_heap_end + increment;
    }
    
    // 确保堆大小不会超过用户空间
    if (new_heap_end > 0xC0000000) {
        return -1;
    }
    
    // 调整堆大小
    current_task->heap_end = new_heap_end;
    
    // 更新内存使用统计
    current_task->memory_usage_kb += (increment + 1023) / 1024;
    
    // 返回旧的堆结束地址
    return old_heap_end;
}

// SYS_sleep - 进程睡眠
int sys_sleep_handler(struct regs* regs) {
    unsigned int seconds = regs->ebx;
    
    // 简化实现：使用时钟中断计数
    uint32_t start_ticks = get_system_ticks();
    uint32_t end_ticks = start_ticks + seconds * 100;
    
    // 将进程状态改为阻塞
    current_task->state = TASK_BLOCKED;
    
    // 循环检查时间，直到超时
    while (get_system_ticks() < end_ticks) {
        schedule();
    }
    
    // 恢复进程状态为就绪
    current_task->state = TASK_READY;
    
    return 0;
}

// SYS_execve - 替换当前进程映像
int sys_execve_handler(struct regs* regs) {
    const char* path = (const char*)regs->ebx;
    char* const* argv = (char* const*)regs->ecx;
    char* const* envp = (char* const*)regs->edx;
    
    uint32_t entry_point;
    
    // 加载ELF文件
    if (elf_load(path, &entry_point) < 0) {
        return -1;
    }
    
    // 设置进程名
    strncpy(current_task->name, path, sizeof(current_task->name) - 1);
    current_task->name[sizeof(current_task->name) - 1] = '\0';
    
    // 设置新的入口点
    current_task->regs.eip = entry_point;
    
    // 简化实现：不处理argv和envp
    
    kprintf("[EXEC] Process %d executed %s, entry at 0x%x\n", 
            current_task->pid, path, entry_point);
    
    return 0;
}

// 初始化系统调用
void syscall_init(void) {
    // 注册系统调用中断处理函数
    register_interrupt_handler(0x80, syscall_handler);
}
