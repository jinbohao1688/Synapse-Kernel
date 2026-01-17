#include <interrupts.h>
#include <keyboard.h>
#include <vga.h>
#include <common.h>

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags)
{
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].zero = 0;
}

void idt_init(void)
{
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_ptr.base = (uint64_t)(uint32_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
}

void keyboard_handler_wrapper(void)
{
    keyboard_handler();
}

void keyboard_handler(void)
{
    key_event_t event;
    
    if (keyboard_read(&event) && event.pressed && event.ascii != 0) {
        vga_putc(event.ascii);
    }
    
    __asm__ volatile("outb %al, $0x20");
}

void isr_install(void)
{
    idt_init();
}

// 声明时钟中断处理函数
void timer_handler_wrapper(void);

void irq_install(void)
{
    uint32_t keyboard_addr = (uint32_t)keyboard_handler_wrapper;
    uint32_t timer_addr = (uint32_t)timer_handler_wrapper;
    
    // 设置键盘中断处理
    idt_set_gate(0x21, (uint64_t)keyboard_addr, 0x08, 0x8E);
    
    // 设置时钟中断处理
    idt_set_gate(0x20, (uint64_t)timer_addr, 0x08, 0x8E);
    
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
    
    // 启用时钟和键盘中断
    __asm__ volatile("inb $0x21, %al");
    __asm__ volatile("andb $0xFC, %al");
    __asm__ volatile("outb %al, $0x21");
}
