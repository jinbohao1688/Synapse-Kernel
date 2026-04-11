.section .text
.global _start

_start:
    mov rsp, __stack_end
    xor ebp, ebp

    xor edi, edi          ; argc = 0
    lea rsi, [rsp + 8]    ; argv = rsp + 8 (after argc, before envp)
    xor edx, edx          ; envp = NULL

    call main

    mov edi, eax
    mov eax, 60           ; syscall number for exit
    syscall

.section .bss
.align 16
.global __stack
__stack:
    .space 4096
__stack_end:
