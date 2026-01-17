#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <stdint.h>
#include <stdbool.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#define KEYBOARD_STATUS_OUTPUT_FULL 0x01
#define KEYBOARD_STATUS_INPUT_FULL 0x02

typedef struct {
    char scancode;
    char ascii;
    bool pressed;
} key_event_t;

void keyboard_init(void);
bool keyboard_read(key_event_t* event);
char keyboard_get_scancode(void);
char scancode_to_ascii(char scancode);

#endif
