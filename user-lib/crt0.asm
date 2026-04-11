section .text
global _start
extern main

; 串口输出一个字符（port 0x3F8）
%macro serial_char 1
    mov al, %1
    out 0x3F8, al
%endmacro

_start:
    ; 打印 "S" 表示 _start 到了
    serial_char 'S'
    serial_char 13
    serial_char 10

    xor rbp, rbp
    xor rdi, rdi
    xor rsi, rsi
    xor rdx, rdx

    serial_char 'C'   ; 即将 call main
    serial_char 13
    serial_char 10

    call main

    ; exit
    mov rdi, rax
    mov rax, 60
    syscall
    hlt
