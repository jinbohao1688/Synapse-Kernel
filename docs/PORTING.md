# Porting Programs to Synapse Kernel

This guide provides instructions on how to port C programs to run on the Synapse Kernel. It covers the key differences between traditional POSIX environments and Synapse Kernel's environment, as well as the steps needed to adapt programs for Synapse.

## Overview of Synapse Kernel Environment

### Key Differences from POSIX

| Aspect               | POSIX Environment         | Synapse Kernel             |
|----------------------|---------------------------|----------------------------|
| C Library            | Full libc implementation  | Minimal user-lib           |
| Memory Model         | Virtual memory with full MMU | Limited virtual memory     |
| File System          | Hierarchical with many types | Simple RamFS implementation|
| System Calls         | Complete set of POSIX syscalls | Subset of 11 syscalls     |
| Dynamic Linking      | Supported                 | Not supported (static linking only) |
| Threading            | pthreads support          | No threading support       |
| Signals              | Full signal handling      | Basic signal support       |

### Memory Layout

Synapse Kernel uses a simple memory layout for user programs:

```
0x00000000 - 0x08048000: Kernel space
0x08048000 - 0xC0000000: User space
    - 0x08048000: Program entry point
    - Dynamic memory: Managed via sbrk() syscall
    - Stack: Fixed-size stack at end of user space
```

## User Program Development Kit

The Synapse Kernel provides a minimal user program development kit in the `user-lib/` directory:

- `user-lib/include/`: User-space header files
- `user-lib/crt0.s`: Program startup code
- `user-lib/linker.ld`: Linker script

### User-Space Headers

The most important headers for porting are:

- `syscall.h`: System call declarations
- `elf.h`: ELF file format definitions (for loading programs)

## Porting Steps

### 1. Analyze the Program

Before porting, analyze the program to identify:

- Dependencies on POSIX functions
- Memory usage patterns
- File I/O operations
- Threading or signal handling

### 2. Adapt the Source Code

#### Replace Standard Library Functions

Synapse Kernel's `user-lib` provides only minimal functionality. Replace standard library functions with their Synapse equivalents:

| Standard Function | Synapse Alternative |
|------------------|---------------------|
| `printf()`       | Use `write()` directly |
| `scanf()`        | Use `read()` directly  |
| `malloc()`       | Implement using `sbrk()` |
| `free()`         | Not supported (implement as no-op) |
| `getchar()`      | Use `read()` on stdin |
| `putchar()`      | Use `write()` on stdout |

#### Example: Replacing printf()

Original POSIX code:
```c
#include <stdio.h>

int main() {
    printf("Hello, world!\n");
    return 0;
}
```

Synapse Kernel adaptation:
```c
#include <syscall.h>

void print(const char* str) {
    write(1, str, strlen(str));
}

int main() {
    print("Hello, Synapse!\n");
    return 0;
}
```

#### Memory Management

Implement custom `malloc()` and `free()` functions using the `sbrk()` syscall:

```c
#include <syscall.h>

void* malloc(size_t size) {
    return sbrk(size);
}

void free(void* ptr) {
    // Simplified: does not actually free memory
}
```

### 3. Compile with Synapse Toolchain

Use the cross-compilation toolchain and Synapse's user-lib to compile the program:

#### Compilation Commands

```bash
# Compile source files
i686-elf-gcc -ffreestanding -nostdlib -O2 -Wall -Wextra -std=gnu99 -Iuser-lib/include -c program.c -o program.o

# Link with user-lib components
i686-elf-ld -T user-lib/linker.ld -nostdlib user-lib/crt0.o program.o -o program
```

#### Using the Apps Build Script

Alternatively, use the provided build script in the `apps/` directory:

```bash
cp program.c apps/program/
cd apps
./build.sh
```

### 4. Test the Program

Run the compiled program in QEMU using the Synapse Kernel:

```bash
# Copy the program to the kernel directory
cp program synapse-kernel/

# Start QEMU
cd synapse-kernel
./tools/qemu-run.sh
```

Once the kernel boots, you can execute the program using the shell.

## Porting Examples

### Example 1: Simple Hello World

```c
#include <syscall.h>

int main() {
    const char* msg = "Hello, Synapse Kernel!\n";
    write(1, msg, 22);
    return 0;
}
```

### Example 2: File I/O Program

```c
#include <syscall.h>

#define O_WRONLY 0x1
#define O_CREAT 0x2
#define S_IRUSR 0x100
#define S_IWUSR 0x80

int main() {
    // Create and write to a file
    int fd = open("test.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
        const char* data = "Synapse Kernel file I/O\n";
        write(fd, data, 23);
        close(fd);
        
        write(1, "File written successfully\n", 25);
    } else {
        write(1, "Failed to open file\n", 20);
    }
    
    return 0;
}
```

### Example 3: Memory Allocation Program

```c
#include <syscall.h>

// Simple malloc implementation
void* malloc(size_t size) {
    return sbrk(size);
}

void free(void* ptr) {
    // No-op implementation
}

int main() {
    // Allocate memory
    char* buffer = (char*)malloc(100);
    if (buffer) {
        write(1, "Memory allocated successfully\n", 30);
        
        // Write to the allocated buffer
        const char* data = "Test data";
        for (int i = 0; i < 9; i++) {
            buffer[i] = data[i];
        }
        buffer[9] = '\0';
        
        write(1, "Buffer content: ", 16);
        write(1, buffer, 9);
        write(1, "\n", 1);
        
        // Free the memory (no-op in this implementation)
        free(buffer);
        write(1, "Memory freed\n", 13);
    } else {
        write(1, "Memory allocation failed\n", 26);
    }
    
    return 0;
}
```

## Common Issues and Solutions

### 1. Missing Header Files

**Issue**: Compiler complains about missing header files like `stdio.h` or `stdlib.h`.

**Solution**: Replace standard library includes with Synapse-specific headers, or implement the required functionality manually.

### 2. Undefined References to Standard Functions

**Issue**: Linker errors about undefined references to functions like `printf` or `malloc`.

**Solution**: Implement custom versions of these functions using Synapse's system calls, or replace them with direct syscall usage.

### 3. Memory Access Violations

**Issue**: Program crashes with a page fault.

**Solution**: Check memory usage patterns. Ensure you're not accessing memory outside the user space range (0x08048000 - 0xC0000000).

### 4. File Operations Fail

**Issue**: File open or write operations return errors.

**Solution**: Check that the file path is correct. Note that Synapse's RamFS is simple and may not support all POSIX file operations.

## Best Practices

1. **Keep it Simple**: Start with small, simple programs before moving to complex ones.
2. **Use Direct Syscalls**: Whenever possible, use direct syscalls instead of standard library functions.
3. **Test Incrementally**: Test each component as you port it, rather than waiting until the entire program is ported.
4. **Avoid Dynamic Features**: Don't use dynamic linking, threading, or other advanced features not supported by Synapse.
5. **Check Memory Usage**: Be mindful of memory constraints and avoid excessive memory allocation.

## Future Enhancements

The Synapse Kernel team plans to add more features to simplify porting:

- Expanded syscall set (including more POSIX syscalls)
- Improved user-lib with more standard functions
- Better file system support
- Basic threading support

## Getting Help

If you encounter issues while porting programs to Synapse Kernel:

1. Check this guide again for any missed steps
2. Review the [SYSCALLS.md](./SYSCALLS.md) to understand available system calls
3. Look at the example programs in the `apps/` directory for reference
4. Create a GitHub issue with details about your problem

For more information about building the kernel itself, refer to the [BUILD.md](./BUILD.md) file.
