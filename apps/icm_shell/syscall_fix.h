#ifndef SYSCALL_FIX_H
#define SYSCALL_FIX_H
#include <stdint.h>

/* Synapse Kernel 系统调用号 */
enum {
    SYS_exit   = 0,
    SYS_fork   = 1,
    SYS_wait   = 2,
    SYS_write  = 3,
    SYS_read   = 4,
    SYS_open   = 5,
    SYS_close  = 6,
    SYS_sbrk   = 9,
    SYS_sleep  = 10,
    SYS_execve = 11
};

enum {
    O_RDONLY = 0,
    O_WRONLY = 1,
    O_RDWR   = 2,
    O_CREAT  = 0x40,
    O_TRUNC  = 0x200
};

/* x86-64 syscall: rax=num, rdi=a1, rsi=a2, rdx=a3 */
static inline long _sys0(long n) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n)
        : "rcx","r11","memory");
    return r;
}
static inline long _sys1(long n, long a) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a)
        : "rcx","r11","memory");
    return r;
}
static inline long _sys2(long n, long a, long b) {
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b)
        : "rcx","r11","memory");
    return r;
}
static inline long _sys3(long n, long a, long b, long c) {
    long r;
    register long _c __asm__("rdx") = c;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "r"(_c)
        : "rcx","r11","memory");
    return r;
}

static inline void sys_exit(int code)       { _sys1(SYS_exit, code); }
static inline int  sys_fork(void)           { return (int)_sys0(SYS_fork); }
static inline int  sys_wait(int *status)    { return (int)_sys1(SYS_wait, (long)status); }
static inline int  sys_write(int fd, const void *buf, int len) {
    return (int)_sys3(SYS_write, fd, (long)buf, len);
}
static inline int  sys_read(int fd, void *buf, int len) {
    return (int)_sys3(SYS_read, fd, (long)buf, len);
}
static inline int  sys_open(const char *path, int flags) {
    return (int)_sys2(SYS_open, (long)path, flags);
}
static inline int  sys_close(int fd)        { return (int)_sys1(SYS_close, fd); }
static inline void*sys_sbrk(long inc)       { return (void*)_sys1(SYS_sbrk, inc); }
static inline int  sys_sleep(unsigned sec)  { return (int)_sys1(SYS_sleep, sec); }
static inline int  sys_execve(const char *path, char **argv, char **envp) {
    return (int)_sys3(SYS_execve, (long)path, (long)argv, (long)envp);
}

#endif
