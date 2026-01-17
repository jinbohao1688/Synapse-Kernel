#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdint.h>
#include <stdbool.h>

#define SERIAL_COM1_BASE 0x3F8
#define SERIAL_DATA_PORT(base) (base)
#define SERIAL_COMMAND_PORT(base) (base + 1)
#define SERIAL_STATUS_PORT(base) (base + 5)

#define SERIAL_DATA_READY 0x01
#define SERIAL_TRANSMITTER_EMPTY 0x20

void serial_init(void);
void serial_write_char(char c);
void serial_write_string(const char* str);
bool serial_can_read(void);
char serial_read_char(void);
bool serial_received(void);

#endif
