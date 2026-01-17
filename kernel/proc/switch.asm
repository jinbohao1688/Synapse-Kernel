; 上下文切换汇编实现
; void switch_to(task_t* old_task, task_t* new_task)

[BITS 32]

; 全局函数声明
global switch_to
global timer_handler_wrapper

extern timer_interrupt_handler

; 上下文切换函数
switch_to:
    ; 保存旧进程上下文到 old_task->regs
    mov eax, [esp + 4]        ; 获取 old_task 指针
    test eax, eax             ; 检查 old_task 是否为 NULL（首次调度）
    jz .no_save               ; 如果为 NULL，跳过保存
    
    ; 保存通用寄存器到 old_task->regs 结构体
    mov [eax + 28], edi       ; regs.edi = edi
    mov [eax + 32], esi       ; regs.esi = esi
    mov [eax + 36], ebp       ; regs.ebp = ebp
    mov [eax + 40], esp       ; regs.esp = esp
    mov [eax + 44], ebx       ; regs.ebx = ebx
    mov [eax + 48], edx       ; regs.edx = edx
    mov [eax + 52], ecx       ; regs.ecx = ecx
    mov [eax + 56], eax       ; regs.eax = eax
    
    ; 保存EIP（返回地址）
    mov ecx, [esp]            ; 获取返回地址
    mov [eax + 60], ecx       ; regs.eip = 返回地址
    
    ; 保存EFLAGS
    pushfd                    ; 将EFLAGS压栈
    pop ecx                   ; 弹出到ecx
    mov [eax + 64], ecx       ; regs.eflags = ecx
    
    ; 保存段寄存器
    mov [eax + 68], cs        ; regs.cs = cs
    mov [eax + 72], ds        ; regs.ds = ds
    mov [eax + 76], es        ; regs.es = es
    mov [eax + 80], fs        ; regs.fs = fs
    mov [eax + 84], gs        ; regs.gs = gs
    mov [eax + 88], ss        ; regs.ss = ss

.no_save:
    ; 恢复新进程上下文从 new_task->regs
    mov eax, [esp + 8]        ; 获取 new_task 指针
    
    ; 切换页目录（关键！实现进程隔离）
    mov ecx, [eax + 24]       ; 获取 new_task->page_dir
    mov cr3, ecx              ; 设置CR3寄存器，切换页目录
    
    ; 恢复通用寄存器
    mov edi, [eax + 28]       ; edi = regs.edi
    mov esi, [eax + 32]       ; esi = regs.esi
    mov ebp, [eax + 36]       ; ebp = regs.ebp
    mov ebx, [eax + 44]       ; ebx = regs.ebx
    mov edx, [eax + 48]       ; edx = regs.edx
    mov ecx, [eax + 52]       ; ecx = regs.ecx
    mov eax, [eax + 56]       ; eax = regs.eax
    
    ; 恢复段寄存器（除CS，由iret恢复）
    mov ds, [eax + 72]        ; ds = regs.ds
    mov es, [eax + 76]        ; es = regs.es
    mov fs, [eax + 80]        ; fs = regs.fs
    mov gs, [eax + 84]        ; gs = regs.gs
    mov ss, [eax + 88]        ; ss = regs.ss
    
    ; 恢复EFLAGS和EIP（通过iret指令）
    push dword [eax + 88]     ; 压入ss
    push dword [eax + 40]     ; 压入esp
    push dword [eax + 64]     ; 压入eflags
    push dword [eax + 68]     ; 压入cs
    push dword [eax + 60]     ; 压入eip
    
    ; 强制返回，恢复上下文
    iret                     ; 从栈中弹出eip、cs、eflags、esp、ss，并跳转

; 时钟中断处理包装函数
timer_handler_wrapper:
    ; 保存所有通用寄存器
    pusha
    
    ; 调用C处理函数
    call timer_interrupt_handler
    
    ; 恢复所有通用寄存器
    popa
    
    ; 发送EOI信号
    mov al, 0x20
    out 0x20, al
    
    ; 中断返回
    iret
