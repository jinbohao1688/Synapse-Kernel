#include <serial.h>
#include <common.h>

static void serial_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t serial_inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void serial_init(void)
{
    serial_outb(SERIAL_COM1_BASE + 1, 0x00);
    serial_outb(SERIAL_COM1_BASE + 3, 0x80);
    serial_outb(SERIAL_COM1_BASE + 0, 0x03);
    serial_outb(SERIAL_COM1_BASE + 1, 0x00);
    serial_outb(SERIAL_COM1_BASE + 3, 0x03);
    serial_outb(SERIAL_COM1_BASE + 2, 0xC7);
    serial_outb(SERIAL_COM1_BASE + 4, 0x0B);
}

void serial_write_char(char c)
{
    while (!(serial_inb(SERIAL_COM1_BASE + 5) & SERIAL_TRANSMITTER_EMPTY)) {
    }
    serial_outb(SERIAL_COM1_BASE, c);
}

void serial_write_string(const char* str)
{
    while (*str != '\0') {
        serial_write_char(*str);
        str++;
    }
}

void serial_write_hex(uint32_t val)
{
    serial_write_char('0');
    serial_write_char('x');
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        serial_write_char(hex_chars[(val >> (i * 4)) & 0xF]);
    }
}

bool serial_can_read(void)
{
    return serial_inb(SERIAL_COM1_BASE + 5) & SERIAL_DATA_READY;
}

char serial_read_char(void)
{
    while (!serial_can_read()) {
        __asm__ volatile("pause");
    }
    return (char)serial_inb(SERIAL_COM1_BASE);
}

bool serial_received(void)
{
    return serial_can_read();
}


