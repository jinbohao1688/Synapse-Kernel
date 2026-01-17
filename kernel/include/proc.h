#ifndef PROC_H
#define PROC_H

#include <stdint.h>
#include <stddef.h>
#include <mm/paging.h>

// 进程状态
typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_ZOMBIE,
    PROCESS_TERMINATED
} process_state_t;

// 进程控制块（PCB）
typedef struct process_control_block {
    uint32_t pid;                    // 进程ID
    process_state_t state;           // 进程状态
    char name[32];                   // 进程名称
    uint32_t* esp;                   // 栈指针
    uint32_t* ebp;                   // 基址指针
    page_directory_t* page_dir;      // 进程页目录
    uint32_t priority;               // 进程优先级
    uint64_t ticks;                  // 进程运行时间片
    struct process_control_block* next;  // 下一个进程（用于链表）
    struct process_control_block* prev;  // 上一个进程（用于链表）
} pcb_t;

// 进程调度器类型
typedef enum {
    SCHEDULER_ROUND_ROBIN,
    SCHEDULER_PRIORITY,
    SCHEDULER_FCFS
} scheduler_type_t;

// 初始化进程管理
extern void init_process_management(void);

// 创建新进程
extern pcb_t* create_process(const char* name, void (*entry_point)(void), uint32_t priority);

// 切换进程
extern void switch_process(pcb_t* next_process);

// 进程调度
extern void schedule(void);

// 终止进程
extern void terminate_process(uint32_t pid);

// 获取当前进程
extern pcb_t* get_current_process(void);

// 设置调度器类型
extern void set_scheduler_type(scheduler_type_t type);

// 进程间通信 - 信号
extern void send_signal(uint32_t pid, int signal);
extern void handle_signal(int signal);

// 进程间通信 - 管道
extern int create_pipe(int pipefd[2]);
extern int pipe_write(int fd, const void* buf, size_t count);
extern int pipe_read(int fd, void* buf, size_t count);

// 内存映射
extern void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int munmap(void* addr, size_t length);

#endif // PROC_H
