#include <proc/task.h>
#include <vga.h>
#include <serial.h>
#include <interrupts.h>

// 测试进程1：打印进程
void printer_process(void)
{
    kprintf("[TEST] Printer process started (PID: %d)\n", get_current_task()->pid);
    
    while (1) {
        // 打印自己的PID
        kprintf("[TEST] Printer process PID: %d\n", get_current_task()->pid);
        
        // 简单的延迟
        for (int i = 0; i < 1000000; i++) {
            __asm__("nop");
        }
    }
}

// 测试进程2：计算进程
void calculator_process(void)
{
    kprintf("[TEST] Calculator process started (PID: %d)\n", get_current_task()->pid);
    
    while (1) {
        // 简单的斐波那契计算
        unsigned int a = 0, b = 1, c;
        for (int i = 0; i < 20; i++) {
            c = a + b;
            a = b;
            b = c;
        }
        
        kprintf("[TEST] Calculator process PID: %d, Fibonacci result: %d\n", get_current_task()->pid, a);
        
        // 简单的延迟
        for (int i = 0; i < 1500000; i++) {
            __asm__("nop");
        }
    }
}

// 初始化测试进程
void init_test_tasks(void)
{
    kprintf("[TEST] Initializing test tasks\n");
    
    // 创建打印进程
    task_t* printer_task = create_task(printer_process, "printer", 5);
    if (!printer_task) {
        kprintf("[ERROR] Failed to create printer task\n");
        return;
    }
    
    // 创建计算进程
    task_t* calculator_task = create_task(calculator_process, "calculator", 5);
    if (!calculator_task) {
        kprintf("[ERROR] Failed to create calculator task\n");
        return;
    }
    
    kprintf("[TEST] Test tasks initialized successfully\n");
}
