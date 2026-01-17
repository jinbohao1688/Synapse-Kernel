#ifndef PROC_TASK_H
#define PROC_TASK_H

#include <stdint.h>
#include <mm/paging.h>

// 进程状态枚举
typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

// 寄存器上下文结构（用于切换）
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} regs_context_t;

// 进程控制块（PCB）
typedef struct task {
    uint32_t pid;                    // 进程ID
    task_state_t state;              // 状态
    uint32_t priority;               // 优先级（0-15）
    
    // 内存管理集成点（关键！）
    page_directory_t* page_dir;      // 页目录指针（利用现有内存保护）
    uint32_t kernel_stack_top;       // 内核栈顶
    uint32_t user_stack_top;         // 用户栈顶
    uint32_t heap_start;             // 堆起始地址
    uint32_t heap_end;               // 堆结束地址
    
    // 上下文
    regs_context_t regs;             // 寄存器保存区
    
    // 调度信息
    uint32_t time_slice;             // 剩余时间片
    uint32_t total_runtime;          // 总运行时间
    uint32_t last_scheduled;         // 上次调度时间戳
    
    // 进程关系
    struct task* parent;             // 父进程
    struct task* children;           // 子进程链表
    struct task* sibling_next;       // 兄弟进程链表
    
    // 资源统计（为AI监控准备）
    uint32_t memory_usage_kb;        // 内存使用（KB）
    char name[32];                   // 进程名
} task_t;

// 最大优先级
#define MAX_PRIORITY 16

// 全局变量声明
extern task_t* current_task;

// 进程管理函数声明
void sched_init(void);
task_t* create_task(void (*entry)(void), const char* name, uint32_t priority);
void schedule(void);
void switch_to(task_t* old_task, task_t* new_task);
void task_exit(int status);
task_t* get_current_task(void);

#endif // PROC_TASK_H
