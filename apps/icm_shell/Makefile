CC      = i686-elf-gcc
LD      = i686-elf-ld
CFLAGS  = -ffreestanding -nostdlib -O2 -std=gnu99 -Wall -Wextra -I../../user-lib/include
LDFLAGS = -T ../../user-lib/linker.ld -nostdlib
CRT0    = ../../user-lib/crt0.o

icm_shell: icm_shell.o icm_lib.o
	$(LD) $(LDFLAGS) $(CRT0) icm_lib.o icm_shell.o -o icm_shell

icm_shell.o: icm_shell.c icm_lib.h syscall_fix.h
	$(CC) $(CFLAGS) -c icm_shell.c -o icm_shell.o

icm_lib.o: icm_lib.c icm_lib.h syscall_fix.h
	$(CC) $(CFLAGS) -c icm_lib.c -o icm_lib.o

clean:
	rm -f *.o icm_shell

.PHONY: clean
