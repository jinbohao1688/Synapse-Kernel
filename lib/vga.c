#include <vga.h>
#include <string.h>

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static size_t vga_row = 0;
static size_t vga_col = 0;
static uint8_t vga_color = VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

void vga_init(void)
{
    vga_clear();
    vga_row = 0;
    vga_col = 0;
    vga_color = VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void vga_set_color(uint8_t fg, uint8_t bg)
{
    vga_color = VGA_COLOR(fg, bg);
}

void vga_putc(char c)
{
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT) {
            vga_scroll();
            vga_row = VGA_HEIGHT - 1;
        }
        return;
    }

    if (c == '\r') {
        vga_col = 0;
        return;
    }

    if (c == '\t') {
        vga_col = (vga_col + 4) & ~3;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
            if (vga_row >= VGA_HEIGHT) {
                vga_scroll();
                vga_row = VGA_HEIGHT - 1;
            }
        }
        return;
    }

    const size_t index = vga_row * VGA_WIDTH + vga_col;
    vga_buffer[index] = VGA_ENTRY(vga_color, c);

    vga_col++;
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT) {
            vga_scroll();
            vga_row = VGA_HEIGHT - 1;
        }
    }
}

void vga_puts(const char* str)
{
    while (*str != '\0') {
        vga_putc(*str);
        str++;
    }
}

void vga_clear(void)
{
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = VGA_ENTRY(vga_color, ' ');
    }
}

void vga_scroll(void)
{
    for (size_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }

    for (size_t i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = VGA_ENTRY(vga_color, ' ');
    }
}

void kprint(const char* str)
{
    vga_puts(str);
}

void kprint_hex(uint32_t value)
{
    const char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[10] = '\0';

    for (int i = 9; i >= 2; i--) {
        buffer[i] = hex_chars[value & 0xF];
        value >>= 4;
    }

    kprint(buffer);
}

void kprint_dec(uint32_t value)
{
    if (value == 0) {
        kprint("0");
        return;
    }

    char buffer[11];
    int i = 10;
    buffer[i] = '\0';

    while (value > 0 && i > 0) {
        i--;
        buffer[i] = '0' + (value % 10);
        value /= 10;
    }

    kprint(&buffer[i]);
}
