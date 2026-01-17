#include <keyboard.h>
#include <common.h>

static const char scancode_to_ascii_table[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_table_shift[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static bool shift_pressed = false;
static bool caps_lock = false;

void keyboard_init(void)
{
    __asm__ volatile("cli");
    
    __asm__ volatile("inb $0x61, %al");
    __asm__ volatile("orb $0x80, %al");
    __asm__ volatile("outb %al, $0x61");
    __asm__ volatile("andb $0x7F, %al");
    __asm__ volatile("outb %al, $0x61");
    
    __asm__ volatile("sti");
}

char keyboard_get_scancode(void)
{
    uint8_t status;
    uint8_t scancode;
    
    do {
        __asm__ volatile("inb $0x64, %0" : "=a"(status));
    } while (!(status & KEYBOARD_STATUS_OUTPUT_FULL));
    
    __asm__ volatile("inb $0x60, %0" : "=a"(scancode));
    
    return scancode;
}

char scancode_to_ascii(char scancode)
{
    char ascii = 0;
    uint8_t code = (uint8_t)scancode;
    
    if (scancode & 0x80) {
        code &= 0x7F;
        
        switch (code) {
            case 0x2A:
            case 0x36:
                shift_pressed = false;
                break;
            case 0x3A:
                caps_lock = !caps_lock;
                break;
        }
    } else {
        switch (code) {
            case 0x2A:
            case 0x36:
                shift_pressed = true;
                break;
            case 0x3A:
                caps_lock = !caps_lock;
                break;
            default:
                if (code < 128) {
                    if (shift_pressed ^ caps_lock) {
                        ascii = scancode_to_ascii_table_shift[code];
                    } else {
                        ascii = scancode_to_ascii_table[code];
                    }
                }
                break;
        }
    }
    
    return ascii;
}

bool keyboard_read(key_event_t* event)
{
    char scancode = keyboard_get_scancode();
    
    event->scancode = scancode;
    event->pressed = !(scancode & 0x80);
    event->ascii = scancode_to_ascii(scancode);
    
    return event->pressed;
}
