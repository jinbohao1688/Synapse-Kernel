#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <proc/regs.h>

// 系统调用号定义
enum syscall_num {
    SYS_exit = 0,
    SYS_fork = 1,
    SYS_wait = 2,
    SYS_write = 3,
    SYS_read = 4,
    SYS_open = 5,
    SYS_close = 6,
    SYS_mmap = 7,
    SYS_munmap = 8,
    SYS_sbrk = 9,
    SYS_sleep = 10,
    SYS_execve = 11
};

// 系统调用处理函数类型
typedef int (*syscall_handler_t)(struct regs* regs);

// 系统调用初始化
extern void syscall_init(void);

// 系统调用处理函数
extern void syscall_handler(struct regs* regs);

// 系统调用入口（用户空间使用）
extern int syscall(int num, ...);

#endif // SYSCALL_H
