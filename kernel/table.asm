; table.asm — x86-64 GDT descriptors + syscall table
; =============================================================================
; GDT layout (selector = index * 8):
;   0x00 — NULL descriptor
;   0x08 — kernel code  (index 1, 64-bit, L=1)
;   0x10 — kernel data  (index 2)
;   0x18 — TSS placeholder (index 3) — unused
;   0x1B — user code   (index 3, RPL=3) — used by IA32_STAR sysret
;   0x23 — user data   (index 4, RPL=3) — used by iretq
;
; IA32_STAR[63:48]=0x1B → sysret loads CS=0x1B (CPL→3)
; IA32_STAR[47:32]=0x08 → sysret loads SS=0x08 (kernel data)
; =============================================================================

[BITS 64]

global gdt64_start
global gdt64_end
global gdt64_ptr
global syscall_table

; ---- 64-bit GDT ----
align 16
gdt64_start:
    dq 0                            ; 0x00 — NULL descriptor
    dq 0x00209A0000000000           ; 0x08 — kernel code (L=1, P=1, DPL=00, type=0xA)
    dq 0x0020920000000000           ; 0x10 — kernel data (P=1, DPL=00, type=0x2)
    dq 0x0000000000000000           ; 0x18 — TSS placeholder (type=0x9, unused)
    ; User code segment (selector 0x1B = index 3, RPL=3)
    ; base=0, limit=0xFFFFF (4GB), G=1 (4K gran), P=1, DPL=3, S=1, type=0xA
    dq 0x00CF9A000000FFFF           ; 0x1B — user code  (CPL=3, 64-bit)
    ; User data segment (selector 0x23 = index 4, RPL=3)
    ; base=0, limit=0xFFFFF (4GB), G=1, P=1, DPL=3, S=1, type=0x2
    dq 0x00CF92000000FFFF           ; 0x23 — user data  (CPL=3)
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

; ---- Syscall table ----
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

syscall_table:
    dq sys_exit_handler     ; 0
    dq sys_fork_handler    ; 1
    dq sys_wait_handler   ; 2
    dq sys_write_handler  ; 3
    dq sys_read_handler   ; 4
    dq sys_open_handler   ; 5
    dq sys_close_handler  ; 6
    dq sys_mmap_handler   ; 7
    dq sys_munmap_handler ; 8
    dq sys_sbrk_handler   ; 9
    dq sys_sleep_handler  ; 10
    dq sys_execve_handler ; 11

; ---- IDT flush ----
section .text
global idt_flush

idt_flush:
    lidt [rdi]
    ret
