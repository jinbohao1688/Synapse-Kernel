; switch.asm — x86-64 context switch for process scheduler
; =============================================================================
; void switch_to(task_t* old_task, task_t* new_task)
; =============================================================================
[BITS 64]

global switch_to
global switch_process
global timer_handler_wrapper

extern timer_interrupt_handler

; Save callee-saved + special registers to old_task->regs
; Restore from new_task->regs
; Stack frame layout on entry:
;   [rsp+8]  = old_task (arg1)
;   [rsp+16] = new_task (arg2)
switch_to:
    mov rax, rdi            ; rax = old_task

    ; Save callee-saved registers to [old_task + offset]
    ; regs.r15 = offset 0
    mov [rax + 0],  r15
    mov [rax + 8],  r14
    mov [rax + 16], r13
    mov [rax + 24], r12
    mov [rax + 32], r11
    mov [rax + 40], r10
    mov [rax + 48], r9
    mov [rax + 56], r8
    mov [rax + 64], rbp
    mov [rax + 72], rdi      ; rdi saved here (it got clobbered)
    mov [rax + 80], rsi
    mov [rax + 88], rdx
    mov [rax + 96], rax      ; original rax
    mov [rax + 104], rcx
    mov [rax + 112], rbx

    ; Save RSP (return address is on top of our hypothetical stack)
    ; We need to save the caller's RSP. Since we're called with call,
    ; the return address is at [rsp] already.
    mov rbx, [rsp]          ; return address = RIP
    mov [rax + 120], rbx    ; regs.rip

    mov rbx, rsp
    add rbx, 8              ; RSP after return addr is pushed
    mov [rax + 128], rbx    ; regs.rsp (caller's rsp)

    ; Save RFLAGS
    pushfq
    pop rbx
    mov [rax + 136], rbx    ; regs.rflags

    ; --- Restore new task ---
    mov rax, rsi            ; rax = new_task

    ; Reload callee-saved registers
    mov r15, [rax + 0]
    mov r14, [rax + 8]
    mov r13, [rax + 16]
    mov r12, [rax + 24]
    mov r11, [rax + 32]
    mov r10, [rax + 40]
    mov r9,  [rax + 48]
    mov r8,  [rax + 56]
    mov rbp, [rax + 64]
    ; Skip rdi/rsi/rbp (restored separately)
    mov rdx, [rax + 88]
    ; rax restored last
    mov rcx, [rax + 104]
    mov rbx, [rax + 112]

    ; Switch page directory
    mov rdx, [rax + 120 + 8]  ; offset for page_dir in task_t (needs alignment)
    mov cr3, rdx

    ; Build iretq stack frame on the new task's kernel stack:
    ;   RSP ← task->regs.rsp
    ;   RIP ← task->regs.rip
    ;   CS  ← 0x08 (kernel code)
    ;   RFLAGS ← task->regs.rflags
    ;   SS  ← 0x10 (kernel data)
    mov rsp, [rax + 128]    ; new RSP

    push qword [rax + 136]  ; RFLAGS
    push qword 0x08         ; CS
    push qword [rax + 120]  ; RIP

    iretq

; switch_process — wrapper that calls switch_to(current_task, next)
switch_process:
    extern current_task
    mov rax, [current_task]
    mov rdx, rax              ; old = current_task
    mov rax, rdi              ; new = arg1 (next_task)
    jmp switch_to

; timer_handler_wrapper — called from IRQ0
timer_handler_wrapper:
    push rbp
    mov rbp, rsp

    ; Save all registers
    push rax; push rcx; push rdx; push rbx
    push rbp; push rsi; push rdi
    push r8;  push r9;  push r10; push r11
    push r12; push r13; push r14; push r15

    call timer_interrupt_handler

    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al

    ; Restore registers
    pop r15; pop r14; pop r13; pop r12
    pop r11; pop r10; pop r9;  pop r8
    pop rdi; pop rsi; pop rbp
    pop rbx; pop rdx; pop rcx; pop rax

    pop rbp
    iretq
