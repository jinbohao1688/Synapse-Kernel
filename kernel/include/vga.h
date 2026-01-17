#ifndef _VGA_H_
#define _VGA_H_

#include <stdint.h>
#include "common.h"

void vga_init(void);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_putc(char c);
void vga_puts(const char* str);
void vga_clear(void);
void vga_scroll(void);

void kprint(const char* str);
void kprint_hex(uint32_t value);
void kprint_dec(uint32_t value);

#endif
