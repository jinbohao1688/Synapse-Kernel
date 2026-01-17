#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define NULL ((void*)0)

#define KERNEL_VERSION "0.1"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

#define VGA_ENTRY(color, c) ((uint16_t)color << 8 | (uint16_t)c)
#define VGA_ENTRY_COLOR(entry) ((entry >> 8) & 0xFF)
#define VGA_ENTRY_CHAR(entry) (entry & 0xFF)

#define VGA_COLOR(fg, bg) ((fg) | (bg << 4))

#define ALIGN_UP(addr, align) (((addr) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define UNUSED(x) (void)(x)

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            while (1) { __asm__ volatile("hlt"); } \
        } \
    } while (0)

#endif
