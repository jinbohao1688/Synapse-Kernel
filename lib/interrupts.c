#include <interrupts.h>
#include <keyboard.h>
#include <vga.h>
#include <common.h>
#include <serial.h>

static void default_handler(void)
{
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags)
{
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector    = sel;
    idt[num].ist         = 0;
    idt[num].type_attr   = flags;
    idt[num].reserved    = 0;
}

void register_interrupt_handler(uint8_t num, void* handler)
{
    idt_set_gate(num, (uint64_t)(uintptr_t)handler, 0x08, IDT_TYPE_INTERRUPT_64);
}

void idt_init(void)
{
    serial_write_string("[IDT] init x86-64 IDT\n");
    serial_write_string("[IDT] sizeof(idt_entry_t)=");
    serial_write_hex64(sizeof(idt_entry_t));
    serial_write_string("\n");

    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_ptr.base  = (uint64_t)(uintptr_t)idt;
    serial_write_string("[IDT] limit=");
    serial_write_hex64(idt_ptr.limit);
    serial_write_string(" base=");
    serial_write_hex64(idt_ptr.base);
    serial_write_string("\n");

    // Fill all entries with default handler
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint64_t)(uintptr_t)default_handler, 0x08, IDT_TYPE_INTERRUPT_64);
    }

    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
    serial_write_string("[IDT] lidt done\n");
}

void keyboard_handler_wrapper(void);

void keyboard_handler(void)
{
    key_event_t event;
    if (keyboard_read(&event) && event.pressed && event.ascii != 0) {
        vga_putc(event.ascii);
    }
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

void keyboard_handler_wrapper(void)
{
    keyboard_handler();
}

void isr_install(void)
{
    idt_init();
}

void irq_install(void)
{
    uint64_t keyboard_addr = (uint64_t)(uintptr_t)keyboard_handler_wrapper;
    uint64_t timer_addr    = (uint64_t)(uintptr_t)keyboard_handler_wrapper; // placeholder

    idt_set_gate(0x21, keyboard_addr, 0x08, IDT_TYPE_INTERRUPT_64);
    idt_set_gate(0x20, timer_addr,    0x08, IDT_TYPE_INTERRUPT_64);

    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    // Unmask PIC IRQ0 (timer) and IRQ1 (keyboard)
    __asm__ volatile(
        "inb $0x21, %%al\n\t"
        "and $0xFC, %%al\n\t"
        "out %%al, $0x21\n\t"
        :
        :
        : "al"
    );
}
