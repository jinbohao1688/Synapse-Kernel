; 系统调用表
; 包含所有系统调用处理函数的指针

[BITS 32]

global syscall_table

extern sys_exit_handler
extern sys_fork_handler
extern sys_wait_handler
extern sys_write_handler
extern sys_read_handler
extern sys_open_handler
extern sys_close_handler
extern sys_mmap_handler
extern sys_munmap_handler
extern sys_sbrk_handler
extern sys_sleep_handler
extern sys_execve_handler

section .data

syscall_table:    ; 系统调用表
    dd sys_exit_handler     ; 0: SYS_exit
    dd sys_fork_handler     ; 1: SYS_fork
    dd sys_wait_handler     ; 2: SYS_wait
    dd sys_write_handler    ; 3: SYS_write
    dd sys_read_handler     ; 4: SYS_read
    dd sys_open_handler     ; 5: SYS_open
    dd sys_close_handler    ; 6: SYS_close
    dd sys_mmap_handler     ; 7: SYS_mmap
    dd sys_munmap_handler   ; 8: SYS_munmap
    dd sys_sbrk_handler     ; 9: SYS_sbrk
    dd sys_sleep_handler    ; 10: SYS_sleep
    dd sys_execve_handler   ; 11: SYS_execve
