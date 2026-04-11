#include <proc/task.h>
#include <mm/paging.h>
#include <mm/kheap.h>
#include <string.h>
#include <vga.h>
#include <serial.h>
#include <interrupts.h>

static void idle_task_fn(void) { while(1) { __asm__("hlt"); } }
static void init_task_fn(void) { while(1) { for(int i=0;i<1000000;i++) __asm__("nop"); } }

// Global variables
task_t* ready_queue[MAX_PRIORITY];  // Priority ready queue
task_t* current_task = NULL;        // Current running process
static uint64_t next_pid = 1;              // Next PID
static uint64_t system_ticks = 0;           // System clock tick counter

// Time slice size (clock interrupt count)
#define TIME_SLICE 10

uint64_t get_system_ticks(void) { return system_ticks; }

task_t* get_current_task(void) { return current_task; }

// Create idle process
static task_t* create_idle_task(void)
{
    serial_write_string("[SCH] kmalloc task\n");
    task_t* idle_task = (task_t*)kmalloc(sizeof(task_t));
    serial_write_string("[SCH] kmalloc done\n");
    if (!idle_task) return NULL;

    serial_write_string("[SCH] memset\n");
    memset(idle_task, 0, sizeof(task_t));
    serial_write_string("[SCH] fields\n");
    idle_task->pid = 0;
    idle_task->state = TASK_READY;
    idle_task->priority = 0;
    serial_write_string("[SCH] get_pml4\n");
    idle_task->page_dir = get_kernel_pml4();
    idle_task->time_slice = TIME_SLICE;
    idle_task->total_runtime = 0;
    idle_task->last_scheduled = 0;
    idle_task->memory_usage_kb = 4;
    strcpy(idle_task->name, "idle");

    void* kernel_stack = kmalloc(4096);
    if (!kernel_stack) { kfree(idle_task); return NULL; }
    idle_task->kernel_stack_top = (uint64_t)kernel_stack + 4096;

    // Set initial context
    idle_task->regs.rip = (uint64_t)(uintptr_t)idle_task_fn;
    idle_task->regs.rsp = idle_task->kernel_stack_top;
    idle_task->regs.rflags = 0x202;   // IF=1
    idle_task->regs.page_dir = (uint64_t)(uintptr_t)get_kernel_pml4();

    return idle_task;
}

// Initialize scheduler
void sched_init(void)
{
    serial_write_string("[LOG] sched_init\n");

    for (int i = 0; i < MAX_PRIORITY; i++) {
        ready_queue[i] = NULL;
    }

    task_t* idle_task = create_idle_task();
    if (!idle_task) { serial_write_string("[LOG] idle failed\n"); return; }

    ready_queue[0] = idle_task;

    task_t* init_task = create_task((void(*)(void))init_task_fn, "init", 5);
    if (!init_task) { serial_write_string("[LOG] init failed\n"); return; }

    ready_queue[5] = init_task;
    current_task = idle_task;

    serial_write_string("[LOG] sched_init done\n");
}

// Create new process
task_t* create_task(void (*entry)(void), const char* name, uint32_t priority)
{
    if (priority >= MAX_PRIORITY) return NULL;

    task_t* task = (task_t*)kmalloc(sizeof(task_t));
    if (!task) return NULL;

    memset(task, 0, sizeof(task_t));
    task->pid = next_pid++;
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = TIME_SLICE;
    task->total_runtime = 0;
    task->last_scheduled = 0;
    task->memory_usage_kb = 8;
    strcpy(task->name, name);

    task->page_dir = (page_entry_t*)kmalloc(sizeof(page_entry_t) * PML4_ENTRIES);
    if (!task->page_dir) { kfree(task); return NULL; }

    // Copy kernel page mappings
    memcpy(task->page_dir, get_kernel_pml4(), sizeof(page_entry_t) * PML4_ENTRIES);

    void* kernel_stack = kmalloc(4096);
    if (!kernel_stack) { kfree(task->page_dir); kfree(task); return NULL; }
    task->kernel_stack_top = (uint64_t)kernel_stack + 4096;

    // Set initial context
    task->regs.rip = (uint64_t)(uintptr_t)entry;
    task->regs.rsp = task->kernel_stack_top;
    task->regs.rflags = 0x202;   // IF=1
    task->regs.page_dir = (uint64_t)(uintptr_t)task->page_dir;

    task->parent = current_task;
    task->sibling_next = ready_queue[priority];
    ready_queue[priority] = task;

    serial_write_string("[LOG] create_task done\n");
    return task;
}

// Schedule algorithm
void schedule(void)
{
    if (!current_task) return;

    current_task->total_runtime++;
    current_task->time_slice--;

    if (current_task->time_slice <= 0) {
        current_task->state = TASK_READY;
        current_task->time_slice = TIME_SLICE;
        task_t* t = current_task;
        t->sibling_next = ready_queue[t->priority];
        ready_queue[t->priority] = t;
        current_task = NULL;
    }

    task_t* next_task = NULL;
    for (int i = MAX_PRIORITY - 1; i >= 0; i--) {
        if (ready_queue[i]) {
            next_task = ready_queue[i];
            ready_queue[i] = next_task->sibling_next;
            next_task->sibling_next = NULL;
            break;
        }
    }

    if (!next_task) {
        next_task = create_idle_task();
        if (!next_task) return;
    }

    next_task->state = TASK_RUNNING;
    next_task->last_scheduled = 0;

    if (current_task != next_task) {
        task_t* old_task = current_task;
        current_task = next_task;
        serial_write_string("[LOG] switch\n");
        switch_process(next_task);
    }
}

// Exit current process
void task_exit(int status)
{
    (void)status;
    serial_write_string("[LOG] task_exit\n");
    current_task->state = TASK_ZOMBIE;
    schedule();
}

// Timer interrupt handler
void timer_interrupt_handler(registers_t* regs)
{
    (void)regs;
    system_ticks++;
    // schedule();  // Disabled: single-process mode
}
