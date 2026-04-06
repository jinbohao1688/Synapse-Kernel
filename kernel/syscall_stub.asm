[BITS 32]
[GLOBAL syscall_stub]
[EXTERN syscall_dispatch]

section .text

syscall_stub:
    ; EAX=num, EBX=arg1, ECX=arg2, EDX=arg3
    push edx        ; arg3
    push ecx        ; arg2
    push ebx        ; arg1
    push eax        ; syscall num
    call syscall_dispatch
    add esp, 16
    iret
