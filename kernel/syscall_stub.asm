; syscall_stub.asm — x86-64 syscall entry via syscall/sysret
; =============================================================================
; syscall instruction lands here.  Arguments arrive in:
;   RAX  = syscall number
;   RDI  = arg1
;   RSI  = arg2
;   RDX  = arg3
;   RCX  = return RIP (saved by CPU, clobbered by 'call')
;   R11  = saved RFLAGS (saved by CPU, clobbered by 'call')
; =============================================================================
[BITS 64]

global syscall_stub

extern syscall_dispatch

section .text

syscall_stub:
    ; Save RCX/R11 BEFORE call clobbers them
    ; (syscall saved them to internal CPU state, but 'call' overwrites RCX)
    push rcx              ; saved_RIP = user RIP to return to
    push r11              ; saved_RFLAGS

    ; Save callee-saved registers
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; RDI/RSI/RDX already hold the 3 arguments
    ; RAX already holds syscall number

    ; Call C dispatcher: syscall_dispatch(num, arg1, arg2, arg3)
    call syscall_dispatch

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ; Restore RCX (= return RIP) and R11 (= saved RFLAGS) for sysretq
    pop r11              ; restore saved RFLAGS
    pop rcx              ; restore saved RIP

    ; sysretq: returns to user mode at RCX:RIP with R11 as RFLAGS
    ; IA32_STAR[63:48] = 0x1B (user CS), IA32_STAR[47:32] = 0x08 (kernel CS)
    ; so sysretq will use CS=0x1B and SS=0x23 automatically
    sysretq
