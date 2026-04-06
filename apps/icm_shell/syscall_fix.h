/*
 * syscall_fix.h — 正确封装 int $0x80，不依赖有 bug 的 libc.c
 * 所有寄存器通过 asm explicit 传入，不依赖 varargs
 */
#ifndef SYSCALL_FIX_H
#define SYSCALL_FIX_H

#include <stdint.h>

/* 系统调用号（与 user-lib/include/syscall.h 保持一致） */
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

/* open flags */
enum {
    O_RDONLY  = 0,
    O_WRONLY  = 1,
    O_RDWR    = 2,
    O_CREAT   = 0x40,
    O_TRUNC   = 0x200
};

/* 通用 0/1/2/3/4 参数 syscall */
static inline int _sys0(int n) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n) : "memory");
    return r;
}

static inline int _sys1(int n, int a) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a) : "memory");
    return r;
}

static inline int _sys2(int n, int a, int b) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b) : "memory");
    return r;
}

static inline int _sys3(int n, int a, int b, int c) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return r;
}

/* 封装各 syscall */
static inline int sys_exit(int code)         { return _sys1(SYS_exit,   code); }
static inline int sys_fork(void)             { return _sys0(SYS_fork); }
static inline int sys_wait(int *status)      { return _sys1(SYS_wait,   (int)status); }
static inline int sys_write(int fd,const void*buf,int len){return _sys3(SYS_write,fd,(int)buf,len);}
static inline int sys_read(int fd,void*buf,int len)      {return _sys3(SYS_read,  fd,(int)buf,len);}
static inline int sys_open(const char*path,int flags)    {return _sys2(SYS_open,  (int)path,flags);}
static inline int sys_close(int fd)          { return _sys1(SYS_close,  fd); }
static inline int sys_sbrk(intptr_t inc)     { return _sys1(SYS_sbrk,   (int)inc); }
static inline int sys_sleep(unsigned sec)    { return _sys1(SYS_sleep,  (int)sec); }
static inline int sys_execve(const char*path,char**argv,char**envp){
    return _sys3(SYS_execve,(int)path,(int)argv,(int)envp);
}

#endif /* SYSCALL_FIX_H */
