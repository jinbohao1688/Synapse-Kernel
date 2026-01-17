#ifndef REGS_H
#define REGS_H

#include <stdint.h>

// 寄存器保存结构，用于上下文切换
// 包含x86架构下所有需要保存的寄存器
struct regs {
    // 通用寄存器（32位）
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    
    // 控制寄存器
    uint32_t eip;
    uint32_t eflags;
    
    // 段寄存器
    uint32_t cs;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t ss;
};

#endif // REGS_H
