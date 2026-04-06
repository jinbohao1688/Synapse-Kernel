section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002
    dd 0x00000003
    dd -(0x1BADB002 + 0x00000003)

section .text
global _start
extern kernel_main

_start:
    cli
    ; QEMU -kernel 已经在保护模式，直接设置栈
    mov esp, stack_top

    ; eax=multiboot magic, ebx=mbi_addr
    push ebx
    push eax
    call kernel_main

    cli
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
