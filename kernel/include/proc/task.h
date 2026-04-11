#ifndef PROC_TASK_H
#define PROC_TASK_H

#include <stdint.h>
#include <mm/paging.h>

// Process state enumeration
typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

// Register context structure (x86-64, must match switch.asm offsets)
typedef struct {
    uint64_t r15, r14, r13, r12;
    uint64_t r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rax, rcx, rbx;
    uint64_t rip, rsp, rflags;
    uint64_t page_dir;
} regs_context_t;

// Process control block (PCB) for x86-64
typedef struct task {
    uint64_t pid;                    // Process ID
    task_state_t state;              // State

    // Memory management
    page_entry_t* page_dir;        // Page map level 4 pointer
    uint64_t kernel_stack_top;       // Kernel stack top
    uint64_t user_stack_top;         // User stack top
    uint64_t heap_start;             // Heap start
    uint64_t heap_end;               // Heap end

    // Context
    regs_context_t regs;             // Register save area

    // Scheduling
    uint32_t priority;               // Priority (0-15)
    uint32_t time_slice;            // Remaining time slice
    uint32_t total_runtime;          // Total runtime
    uint32_t last_scheduled;         // Last scheduled timestamp

    // Process relationships
    struct task* parent;             // Parent process
    struct task* children;           // Child process list
    struct task* sibling_next;       // Sibling list

    // Resource statistics
    uint32_t memory_usage_kb;        // Memory usage (KB)
    char name[32];                   // Process name
} task_t;

// Maximum priority
#define MAX_PRIORITY 16

// Global variable declarations
extern task_t* current_task;
extern task_t* ready_queue[MAX_PRIORITY];

// Process management functions
void sched_init(void);
task_t* create_task(void (*entry)(void), const char* name, uint32_t priority);
void schedule(void);
void switch_to(task_t* old_task, task_t* new_task);
void task_exit(int status);
task_t* get_current_task(void);

#endif // PROC_TASK_H
