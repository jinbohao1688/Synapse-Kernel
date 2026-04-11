section .multiboot
align 4
dd 0x1BADB002
dd 0x00000003
dd -(0x1BADB002 + 0x00000003)

section .bss
align 16
stack_bottom: resb 16384
stack_top:

section .data
align 16
gdt64:
    .null:  dq 0
    .code:  dq (1<<44)|(1<<47)|(1<<41)|(1<<43)|(1<<53)
    .data:  dq (1<<44)|(1<<47)|(1<<41)
    .pointer:
        dw $ - gdt64 - 1
        dq gdt64

section .text
bits 32
global _start
extern kernel_main

; 页表放在固定物理地址（与 paging.c 的 PML4_PHYS=0x110000 一致）
%define BOOT_PML4  0x110000
%define BOOT_PDPT  0x111000
%define BOOT_PD    0x112000

_start:
    cli

    ; 清零页表区域（3 pages = 0x3000 bytes）
    mov edi, BOOT_PML4
    mov ecx, (0x3000 / 4)
    xor eax, eax
    rep stosd

    ; PML4[0] -> PDPT (identity map)
    mov eax, BOOT_PDPT
    or  eax, 0x3
    mov [BOOT_PML4], eax

    ; PDPT[0] -> PD
    mov eax, BOOT_PD
    or  eax, 0x3
    mov [BOOT_PDPT], eax

    ; PD[0..511] -> 2MB pages (PSE)
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

    ; CR3 = PML4
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
