#include <stddef.h>
#include <syscall.h>

static long syscall(long num, long arg1, long arg2, long arg3)
{
    long ret;
    __asm__ volatile(
        "mov rax, %0\n\t"
        "mov rdi, %1\n\t"
        "mov rsi, %2\n\t"
        "mov rdx, %3\n\t"
        "syscall"
        : "=a"(ret)
        : "r"(arg1), "r"(arg2), "r"(arg3)
        : "memory", "rcx", "r11"
    );
    return ret;
}

ssize_t write(int fd, const void* buf, size_t count)
{
    return (ssize_t)syscall(SYS_write, fd, (long)buf, (long)count);
}

ssize_t read(int fd, void* buf, size_t count)
{
    return (ssize_t)syscall(SYS_read, fd, (long)buf, (long)count);
}

int open(const char* path, int flags)
{
    return (int)syscall(SYS_open, (long)path, flags, 0);
}

int close(int fd)
{
    return (int)syscall(SYS_close, fd, 0, 0);
}

void* sbrk(intptr_t increment)
{
    return (void*)syscall(SYS_sbrk, (long)increment, 0, 0);
}

int sleep(unsigned int seconds)
{
    return (int)syscall(SYS_sleep, seconds, 0, 0);
}

void* malloc(size_t size)
{
    size = (size + 4095) & ~4095;
    return sbrk((intptr_t)size);
}

void free(void* ptr)
{
    (void)ptr;
}

int printf(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    int i = 0;

    va_start(args, format);

    while (*format && i < 1023) {
        if (*format == '%') {
            format++;

            switch (*format) {
                case 'd': {
                    int val = va_arg(args, int);
                    char num_buf[32];
                    int j = 0;
                    int is_neg = 0;

                    if (val < 0) {
                        is_neg = 1;
                        val = -val;
                    }

                    do {
                        num_buf[j++] = '0' + (val % 10);
                        val /= 10;
                    } while (val > 0 && j < 30);

                    if (is_neg) num_buf[j++] = '-';

                    for (int k = 0; k < j / 2; k++) {
                        char temp = num_buf[k];
                        num_buf[k] = num_buf[j - k - 1];
                        num_buf[j - k - 1] = temp;
                    }
                    num_buf[j] = '\0';

                    for (int k = 0; k < j && i < 1023; k++)
                        buffer[i++] = num_buf[k];
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    const char* hex = "0123456789abcdef";
                    buffer[i++] = '0';
                    buffer[i++] = 'x';

                    for (int j = 28; j >= 0; j -= 4)
                        buffer[i++] = hex[(val >> j) & 0xf];
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    while (*str && i < 1023)
                        buffer[i++] = *str++;
                    break;
                }
                case 'c': {
                    buffer[i++] = (char)va_arg(args, int);
                    break;
                }
                default:
                    buffer[i++] = *format;
                    break;
            }
        } else {
            buffer[i++] = *format;
        }
        format++;
    }

    buffer[i] = '\0';
    va_end(args);
    return write(1, buffer, i);
}

void exit(int status)
{
    syscall(SYS_exit, status, 0, 0);
    __asm__ volatile("hlt");
}
