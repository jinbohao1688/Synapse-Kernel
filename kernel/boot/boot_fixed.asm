section .multiboot
align 4

multiboot_header:
    dd 0x1BADB002
    dd -(0x1BADB002 + 0x03 + 0)
    dd 0

section .text
global _start
extern kernel_entry

_start:
    cli
    lgdt [gdt_descriptor]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp CODE_SEG:init_pm

init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov esp, stack_top
    
    push ebx
    push eax
    
    call kernel_entry
    
    cli
.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .rodata
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

gdt_code:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 0x0

gdt_data:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
    dw 0x9A00

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
