# Synapse Kernel System Calls

This document provides a comprehensive reference for all system calls implemented in the Synapse Kernel.

## System Call Overview

| Number | Name       | Prototype                                                | Function                                      | Return Value                  |
|--------|------------|---------------------------------------------------------|-----------------------------------------------|-------------------------------|
| 0      | SYS_exit   | `int exit(int status)`                                  | Terminate the current process                 | Never returns                 |
| 1      | SYS_fork   | `pid_t fork(void)`                                      | Create a new process by duplicating the caller| Child: 0, Parent: Child PID   |
| 2      | SYS_wait   | `pid_t wait(pid_t pid)`                                 | Wait for a child process to terminate         | Child PID on success          |
| 3      | SYS_write  | `ssize_t write(int fd, const void* buf, size_t count)`  | Write to a file descriptor                    | Number of bytes written       |
| 4      | SYS_read   | `ssize_t read(int fd, void* buf, size_t count)`         | Read from a file descriptor                   | Number of bytes read          |
| 5      | SYS_open   | `int open(const char* path, int flags, ...)`            | Open a file or device                         | File descriptor on success    |
| 6      | SYS_close  | `int close(int fd)`                                     | Close a file descriptor                       | 0 on success                 |
| 7      | SYS_mmap   | `void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)` | Map files or devices into memory | Pointer to mapped area        |
| 8      | SYS_munmap | `int munmap(void* addr, size_t length)`                 | Unmap memory pages                            | 0 on success                 |
| 9      | SYS_sbrk   | `void* sbrk(intptr_t increment)`                        | Adjust process heap size                      | Previous heap end pointer     |
| 10     | SYS_sleep  | `int sleep(unsigned int seconds)`                       | Suspend process execution                     | 0 on success                 |
| 11     | SYS_execve | `int execve(const char* path, char* const argv[], char* const envp[])` | Execute a program | Never returns on success      |

## Detailed System Call Reference

### SYS_exit (0)

**Prototype**: `int exit(int status)`

**Function**: Terminates the current process and returns a status code to the parent process.

**Parameters**:
- `status`: Exit status code (0 for success, non-zero for failure)

**Return Value**: Never returns

**Example**:
```c
#include <syscall.h>

int main() {
    exit(0); // Normal exit
    return 0; // Unreachable
}
```

### SYS_fork (1)

**Prototype**: `pid_t fork(void)`

**Function**: Creates a new process by duplicating the calling process. The new process (child) is an exact copy of the calling process (parent), except for its PID, parent PID, and resource statistics.

**Return Value**:
- Child process: Returns 0
- Parent process: Returns the child PID
- On failure: Returns -1

**Example**:
```c
#include <stdio.h>
#include <syscall.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Child process\n");
        exit(0);
    } else if (pid > 0) {
        printf("Parent process, child PID: %d\n", pid);
    } else {
        printf("Fork failed\n");
    }
    
    return 0;
}
```

### SYS_wait (2)

**Prototype**: `pid_t wait(pid_t pid)`

**Function**: Waits for a child process to terminate and returns the PID of the terminated child.

**Parameters**:
- `pid`: PID of the child process to wait for, or -1 to wait for any child

**Return Value**:
- On success: Returns the PID of the terminated child
- On failure: Returns -1

**Example**:
```c
#include <stdio.h>
#include <syscall.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        sleep(2);
        exit(0);
    } else if (pid > 0) {
        // Parent process
        printf("Waiting for child...\n");
        pid_t waited_pid = wait(pid);
        printf("Child %d terminated\n", waited_pid);
    }
    
    return 0;
}
```

### SYS_write (3)

**Prototype**: `ssize_t write(int fd, const void* buf, size_t count)`

**Function**: Writes data to a file descriptor.

**Parameters**:
- `fd`: File descriptor (1 for stdout, 2 for stderr)
- `buf`: Pointer to the data buffer
- `count`: Number of bytes to write

**Return Value**:
- On success: Number of bytes written
- On failure: Returns -1

**Example**:
```c
#include <syscall.h>

int main() {
    const char* message = "Hello, Synapse Kernel!\n";
    write(1, message, 22); // Write to stdout
    return 0;
}
```

### SYS_read (4)

**Prototype**: `ssize_t read(int fd, void* buf, size_t count)`

**Function**: Reads data from a file descriptor.

**Parameters**:
- `fd`: File descriptor (0 for stdin)
- `buf`: Pointer to the buffer to store data
- `count`: Maximum number of bytes to read

**Return Value**:
- On success: Number of bytes read
- On failure: Returns -1

**Example**:
```c
#include <syscall.h>

int main() {
    char buffer[100];
    read(0, buffer, 99); // Read from stdin
    write(1, buffer, 99); // Echo back to stdout
    return 0;
}
```

### SYS_open (5)

**Prototype**: `int open(const char* path, int flags, ...)`

**Function**: Opens a file or device.

**Parameters**:
- `path`: Path to the file or device
- `flags`: Opening flags (O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, etc.)
- `mode`: File permissions (when using O_CREAT)

**Return Value**:
- On success: File descriptor
- On failure: Returns -1

**Example**:
```c
#include <syscall.h>

#define O_WRONLY 0x1
#define O_CREAT 0x2
#define S_IRUSR 0x100
#define S_IWUSR 0x80

int main() {
    int fd = open("test.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
        write(fd, "Hello", 5);
        close(fd);
    }
    return 0;
}
```

### SYS_close (6)

**Prototype**: `int close(int fd)`

**Function**: Closes a file descriptor.

**Parameters**:
- `fd`: File descriptor to close

**Return Value**:
- On success: 0
- On failure: Returns -1

**Example**:
```c
#include <syscall.h>

int main() {
    int fd = open("test.txt", 0);
    if (fd >= 0) {
        close(fd);
    }
    return 0;
}
```

### SYS_mmap (7)

**Prototype**: `void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)`

**Function**: Maps files or devices into memory.

**Parameters**:
- `addr`: Desired starting address (NULL for automatic allocation)
- `length`: Length of the mapping
- `prot`: Memory protection flags (PROT_READ, PROT_WRITE, PROT_EXEC)
- `flags`: Mapping flags (MAP_SHARED, MAP_PRIVATE, etc.)
- `fd`: File descriptor
- `offset`: Offset into the file

**Return Value**:
- On success: Pointer to the mapped area
- On failure: Returns NULL

**Example**:
```c
#include <syscall.h>

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2

int main() {
    void* addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    if (addr != NULL) {
        *(char*)addr = 'H';
        munmap(addr, 4096);
    }
    return 0;
}
```

### SYS_munmap (8)

**Prototype**: `int munmap(void* addr, size_t length)`

**Function**: Unmaps memory pages.

**Parameters**:
- `addr`: Starting address of the mapping
- `length`: Length of the mapping

**Return Value**:
- On success: 0
- On failure: Returns -1

**Example**:
```c
#include <syscall.h>

int main() {
    void* addr = mmap(NULL, 4096, 0x3, 0x2, -1, 0);
    if (addr != NULL) {
        munmap(addr, 4096);
    }
    return 0;
}
```

### SYS_sbrk (9)

**Prototype**: `void* sbrk(intptr_t increment)`

**Function**: Adjusts the size of the process's heap.

**Parameters**:
- `increment`: Number of bytes to increase the heap size by

**Return Value**:
- On success: Previous heap end pointer
- On failure: Returns NULL

**Example**:
```c
#include <syscall.h>

void* malloc(size_t size) {
    return sbrk(size);
}

void free(void* ptr) {
    // Simplified implementation: does not actually free memory
}

int main() {
    char* buf = (char*)malloc(100);
    if (buf) {
        buf[0] = 'A';
        free(buf);
    }
    return 0;
}
```

### SYS_sleep (10)

**Prototype**: `int sleep(unsigned int seconds)`

**Function**: Suspends process execution for the specified number of seconds.

**Parameters**:
- `seconds`: Number of seconds to sleep

**Return Value**:
- On success: 0
- On failure: Returns -1

**Example**:
```c
#include <syscall.h>

int main() {
    write(1, "Sleeping for 5 seconds...\n", 24);
    sleep(5);
    write(1, "Awake!\n", 7);
    return 0;
}
```

### SYS_execve (11)

**Prototype**: `int execve(const char* path, char* const argv[], char* const envp[])`

**Function**: Replaces the current process image with a new program.

**Parameters**:
- `path`: Path to the executable file
- `argv`: Command-line arguments
- `envp`: Environment variables

**Return Value**:
- On success: Never returns
- On failure: Returns -1

**Example**:
```c
#include <syscall.h>

int main() {
    char* argv[] = {"/bin/sh", NULL};
    execve("/bin/sh", argv, NULL);
    return 0; // Only reached on error
}
```

## Usage Notes

1. **System Call Entry**: In user space, system calls are invoked using the `syscall` function or by directly using assembly `int $0x80` instruction.

2. **Error Handling**: Most system calls return -1 on failure. The actual error code is typically stored in a global variable `errno` (not yet implemented in Synapse Kernel).

3. **File Descriptors**: The kernel maintains a per-process file descriptor table. Standard file descriptors are:
   - 0: Standard input (stdin)
   - 1: Standard output (stdout)
   - 2: Standard error (stderr)

4. **Memory Protection**: The kernel enforces memory protection through paging. Attempting to access protected memory will result in a page fault.

5. **Process Management**: The kernel uses a round-robin scheduler to manage process execution.

## Future System Calls

The following system calls are planned for future implementation:

- `SYS_kill`: Send a signal to a process
- `SYS_getpid`: Get the process ID
- `SYS_getppid`: Get the parent process ID
- `SYS_stat`: Get file status
- `SYS_lseek`: Change file offset
- `SYS_mkdir`: Create a directory
- `SYS_rmdir`: Remove a directory
- `SYS_unlink`: Delete a file

For more information about the kernel's architecture and implementation, please refer to the [README.md](../README.md) and [BUILD.md](./BUILD.md) files.
