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

bool serial_can_read(void)
{
    return serial_inb(SERIAL_COM1_BASE + 5) & SERIAL_DATA_READY;
}

char serial_read_char(void)
{
    while (!serial_can_read()) {
    }
    return serial_inb(SERIAL_COM1_BASE);
}

bool serial_received(void)
{
    return serial_can_read();
}
