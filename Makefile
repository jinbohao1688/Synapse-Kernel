# Synapse Kernel Makefile
# World's first AI-native operating system kernel

# Default target
all: kernel

# Cross-compilation toolchain prefix
CROSS_COMPILE ?= i686-elf-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AS := $(CROSS_COMPILE)as
NASM := nasm

# Compiler flags
CFLAGS := -ffreestanding -nostdlib -O0 -Wall -Wextra -std=gnu99 -Ikernel/include
LDFLAGS := -T linker.ld -nostdlib
NASMFLAGS := -f elf32

# Kernel source files
BOOT_SOURCES := kernel/boot/boot.asm
ASM_SOURCES := kernel/proc/switch.asm kernel/table.asm kernel/syscall_stub.asm
C_SOURCES := kernel/main.c \
            kernel/mm/kheap.c \
            kernel/mm/paging.c \
            kernel/proc/process.c \
            kernel/proc/sched.c \
            kernel/proc/procfs.c \
            kernel/fs/vfs.c \
            kernel/fs/ramfs.c \
            kernel/fs/tmpfs.c \
            kernel/loader/elf.c \
            kernel/syscall.c \
            kernel/executor.c

KERNEL_SOURCES := $(C_SOURCES) $(ASM_SOURCES)

# Library source files
LIB_SOURCES := kernel/../lib/string.c \
               kernel/../lib/mem.c \
               kernel/../lib/vga.c \
               kernel/../lib/serial.c \
               kernel/../lib/keyboard.c \
               kernel/../lib/interrupts.c \
               kernel/../lib/shell.c \
               kernel/../lib/kprintf.c

# Object files
BOOT_OBJECTS := $(BOOT_SOURCES:.asm=.o)
ASM_OBJECTS := $(ASM_SOURCES:.asm=.o)
C_OBJECTS := $(C_SOURCES:.c=.o)
KERNEL_OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)
LIB_OBJECTS := $(LIB_SOURCES:.c=.o)

OBJECTS := $(BOOT_OBJECTS) $(KERNEL_OBJECTS) $(LIB_OBJECTS)

# Targets
TARGET := synapse.bin

# Linker script
LINKER_SCRIPT := linker.ld

# Include linker script

# Build kernel
kernel: $(OBJECTS) $(LINKER_SCRIPT)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Kernel built successfully: $(TARGET)"

# Assemble boot files
%.o: %.asm
	@echo "Assembling $<..."
	$(NASM) $(NASMFLAGS) $< -o $@

# Special rules for nasm assembly files
kernel/proc/switch.o: kernel/proc/switch.asm
	@echo "Assembling $<..."
	$(NASM) $(NASMFLAGS) $< -o $@

kernel/table.o: kernel/table.asm
	@echo "Assembling $<..."
	$(NASM) $(NASMFLAGS) $< -o $@

# Assemble .S (preprocessed asm) files
%.o: %.S
	@echo "Assembling $<..."
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

# Compile C files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Build user applications
apps: 
	@echo "Building user applications..."
	@cd apps && ./build.sh

# Clean build artifacts
clean: 
	@echo "Cleaning build artifacts..."
	@rm -f $(OBJECTS) $(TARGET)
	@rm -rf apps/bin/*
	@rm -f kernel/boot/*.o kernel/mm/*.o kernel/proc/*.o kernel/fs/*.o kernel/loader/*.o kernel/*.o
	@echo "Clean complete"

# Run kernel in QEMU
run: $(TARGET)
	@echo "Running kernel in QEMU..."
	@qemu-system-i386 -nographic -kernel $(TARGET) -m 128M -monitor none -serial stdio

# Run kernel in QEMU with debug output
debug: $(TARGET)
	@echo "Running kernel in QEMU with debug output..."
	@qemu-system-i386 -nographic -kernel $(TARGET) -m 128M -monitor none -serial stdio -d int -no-reboot

# Run kernel in QEMU with GDB support
gdb: $(TARGET)
	@echo "Running kernel in QEMU with GDB support..."
	@qemu-system-i386 -nographic -kernel $(TARGET) -m 128M -monitor none -serial stdio -s -S

# Help target
help: 
	@echo "Synapse Kernel Build System"
	@echo "============================"
	@echo "Available targets:"
	@echo "  all       : Build kernel (default)"
	@echo "  kernel    : Build only the kernel"
	@echo "  apps      : Build user applications"
	@echo "  clean     : Clean build artifacts"
	@echo "  run       : Run kernel in QEMU"
	@echo "  debug     : Run kernel in QEMU with debug output"
	@echo "  gdb       : Run kernel in QEMU with GDB support"
	@echo "  help      : Show this help message"
	@echo ""
	@echo "Environment variables:"
	@echo "  CROSS_COMPILE : Cross-compiler prefix (default: i686-elf-)"

.PHONY: all kernel apps clean run debug gdb help
