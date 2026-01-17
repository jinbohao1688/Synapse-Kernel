.section .text
.global _start

_start:
    # Initialize stack pointer
    movl $__stack_end, %esp
    
    # Clear EBP (frame pointer)
    xorl %ebp, %ebp
    
    # Push argc and argv (simulated for now)
    xorl %eax, %eax
    pushl %eax  # argv[0] = NULL (end of argv)
    pushl %eax  # argv = NULL (no arguments)
    pushl %eax  # argc = 0
    
    # Call main()
    call main
    
    # Exit with main's return value
    movl %eax, %ebx
    movl $1, %eax  # syscall: exit
    int $0x80

.section .bss
.align 4
.global __stack
__stack:
    .space 4096  # 4KB stack
__stack_end:
