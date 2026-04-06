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
        serial_write_string("[LOG]\n");
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
    
    serial_write_string("[LOG]\n");
}

// 创建新进程
pcb_t* create_process(const char* name, void (*entry_point)(void), uint32_t priority) {
    // 分配PCB
    pcb_t* process = (pcb_t*)kmalloc(sizeof(pcb_t));
    if (!process) {
        serial_write_string("[LOG]\n");
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
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    // 复制内核页目录映射
    memcpy(process->page_dir, get_kernel_page_dir(), sizeof(page_directory_t));
    
    // 分配用户栈
    uint32_t* stack = (uint32_t*)kmalloc(8192); // 8KB栈
    if (!stack) {
        kfree(process->page_dir);
        kfree(process);
        serial_write_string("[LOG]\n");
        return NULL;
    }
    
    // 设置栈指针（指向栈顶）
    process->esp = stack + 8192 / sizeof(uint32_t);
    process->ebp = process->esp;
    
    // 将进程添加到链表
    add_process_to_list(process);
    
    serial_write_string("[LOG]\n");
    return process;
}

// 终止进程
void terminate_process(uint32_t pid) {
    if (pid == 0) {
        serial_write_string("[LOG]\n");
        return;
    }
    
    pcb_t* process = process_list;
    do {
        if (process->pid == pid) {
            process->state = PROCESS_TERMINATED;
            serial_write_string("[LOG]\n");
            
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
    
    serial_write_string("[LOG]\n");
}

// 获取当前进程
pcb_t* get_current_process(void) {
    return current_process;
}

// 设置调度器类型
void set_scheduler_type(scheduler_type_t type) {
    current_scheduler = type;
    const char* scheduler_names[] = {"Round Robin", "Priority", "FCFS"};
    serial_write_string("[LOG]\n");
}

// 信号处理
void send_signal(uint32_t pid, int signal) {
    // 简化实现：仅记录信号
    serial_write_string("[LOG]\n");
}

void handle_signal(int signal) {
    // 简化实现：仅打印信号
    serial_write_string("[LOG]\n");
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
    serial_write_string("[LOG]\n");
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
    
    serial_write_string("[LOG]\n");
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
    
    serial_write_string("[LOG]\n");
    return 0;
}
