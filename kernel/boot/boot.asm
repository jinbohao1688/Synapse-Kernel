; Multiboot2 header
section .multiboot2
align 8
mb2_header_start:
    dd 0xE85250D6                                    ; magic
    dd 0                                             ; arch: i386 protected mode
    dd mb2_header_end - mb2_header_start             ; length
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start)) ; checksum
    ; end tag
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

section .bss
align 16
stack_bottom: resb 16384
stack_top:

section .data
align 16
gdt64:
    .null:      dq 0                                          ; 0x00 null
    .code:      dq (1<<44)|(1<<47)|(1<<41)|(1<<43)|(1<<53)  ; 0x08 kernel code
    .data:      dq (1<<44)|(1<<47)|(1<<41)                   ; 0x10 kernel data
    .ucode:     dq (1<<44)|(1<<47)|(1<<41)|(1<<43)|(1<<53)|(3<<45) ; 0x18 user code  RPL=3
    .udata:     dq (1<<44)|(1<<47)|(1<<41)|(3<<45)           ; 0x20 user data  RPL=3
    .pointer:
        dw $ - gdt64 - 1
        dq gdt64

section .text
bits 32
global _start
extern kernel_main

%define BOOT_PML4  0x110000
%define BOOT_PDPT  0x111000
%define BOOT_PD    0x112000

_start:
    cli

    ; 清零页表
    mov edi, BOOT_PML4
    mov ecx, (0x3000 / 4)
    xor eax, eax
    rep stosd

    ; PML4[0] -> PDPT
    mov eax, BOOT_PDPT
    or  eax, 0x3
    mov [BOOT_PML4], eax

    ; PDPT[0] -> PD
    mov eax, BOOT_PD
    or  eax, 0x3
    mov [BOOT_PDPT], eax

    ; PD: 512 x 2MB pages
    mov ecx, 0
.fill_pd:
    mov eax, ecx
    shl eax, 21
    or  eax, 0x83
    mov [BOOT_PD + ecx*8], eax
    inc ecx
    cmp ecx, 512
    jl  .fill_pd

    ; PAE
    mov eax, cr4
    or  eax, (1<<5)
    mov cr4, eax

    ; CR3
    mov eax, BOOT_PML4
    mov cr3, eax

    ; EFER.LME
    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1<<8)
    wrmsr

    ; PG + PE
    mov eax, cr0
    or  eax, (1<<31)|(1<<0)
    mov cr0, eax

    lgdt [gdt64.pointer]
    jmp  0x08:.long_mode

bits 64
.long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, stack_top
    call kernel_main
    cli
.halt:
    hlt
    jmp .halt
