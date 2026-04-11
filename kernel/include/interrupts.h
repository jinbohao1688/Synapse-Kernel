#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include <stdint.h>

#define IDT_ENTRIES 256

// x86-64 IDT gate descriptor (16 bytes)
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;        // IST (3 bits) + zero (5 bits)
    uint8_t  type_attr;  // P(1) + DPL(2) + S(1) + Type(4)
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

// x86-64 IDT pointer (10 bytes)
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

// x86-64 interrupt frame pushed by CPU
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} registers_t;

// IDT gate types
#define IDT_TYPE_INTERRUPT_64  0x8E  // 64-bit interrupt gate (Present, DPL=00, Type=1110)
#define IDT_TYPE_TRAP_64       0x8F  // 64-bit trap gate

void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);
void isr_install(void);
void irq_install(void);

void keyboard_handler(void);
void keyboard_handler_wrapper(void);

void register_interrupt_handler(uint8_t num, void* handler);

#endif
