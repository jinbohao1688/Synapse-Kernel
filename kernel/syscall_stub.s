.section .text
.global syscall_stub

syscall_stub:
    pusha                   /* 压 edi,esi,ebp,esp,ebx,edx,ecx,eax */
    push %ds
    push %es
    push %fs
    push %gs

    mov $0x10, %ax          /* 内核数据段 */
    mov %ax, %ds
    mov %ax, %es

    push %esp               /* 把 struct regs* 作为参数传入 */
    call syscall_handler
    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    iret
