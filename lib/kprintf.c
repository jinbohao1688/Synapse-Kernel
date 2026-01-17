#include <stdarg.h>
#include <string.h>
#include <vga.h>
#include <fs.h>

// 简单的格式化输出缓冲区大小
#define PRINTF_BUFFER_SIZE 1024

// 内核格式化输出函数
int kprintf(const char* format, ...)
{
    char buffer[PRINTF_BUFFER_SIZE];
    va_list args;
    int printed = 0;
    int i = 0;
    
    va_start(args, format);
    
    // 简单的格式化输出实现
    while (*format && i < PRINTF_BUFFER_SIZE - 1) {
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
                    
                    if (is_neg) {
                        num_buf[j++] = '-';
                    }
                    
                    // 反转字符串
                    for (int k = 0; k < j / 2; k++) {
                        char temp = num_buf[k];
                        num_buf[k] = num_buf[j - k - 1];
                        num_buf[j - k - 1] = temp;
                    }
                    num_buf[j] = '\0';
                    
                    // 复制到主缓冲区
                    for (int k = 0; k < j && i < PRINTF_BUFFER_SIZE - 1; k++) {
                        buffer[i++] = num_buf[k];
                    }
                    break;
                }
                case 'x': {
                    uint32_t val = va_arg(args, uint32_t);
                    const char* hex = "0123456789abcdef";
                    buffer[i++] = '0';
                    buffer[i++] = 'x';
                    
                    for (int j = 28; j >= 0; j -= 4) {
                        char c = hex[(val >> j) & 0xf];
                        buffer[i++] = c;
                    }
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) {
                        str = "(null)";
                    }
                    
                    while (*str && i < PRINTF_BUFFER_SIZE - 1) {
                        buffer[i++] = *str++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    buffer[i++] = c;
                    break;
                }
                default: {
                    buffer[i++] = *format;
                    break;
                }
            }
        } else {
            buffer[i++] = *format;
        }
        format++;
    }
    
    buffer[i] = '\0';
    printed = i;
    
    va_end(args);
    
    // 输出到VGA控制台
    kprint(buffer);
    
    // 尝试写入到/dev/log文件（如果存在）
    int log_fd = open("/dev/log", O_WRONLY | O_CREAT, 0666);
    if (log_fd >= 0) {
        write(log_fd, buffer, strlen(buffer));
        close(log_fd);
    }
    
    return printed;
}

// 简单的snprintf实现
int snprintf(char* str, size_t size, const char* format, ...)
{
    va_list args;
    int i = 0;
    
    va_start(args, format);
    
    // 简单的格式化输出实现
    while (*format && i < size - 1) {
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
                    
                    if (is_neg) {
                        num_buf[j++] = '-';
                    }
                    
                    // 反转字符串
                    for (int k = 0; k < j / 2; k++) {
                        char temp = num_buf[k];
                        num_buf[k] = num_buf[j - k - 1];
                        num_buf[j - k - 1] = temp;
                    }
                    num_buf[j] = '\0';
                    
                    // 复制到主缓冲区
                    for (int k = 0; k < j && i < size - 1; k++) {
                        str[i++] = num_buf[k];
                    }
                    break;
                }
                case 'x': {
                    uint32_t val = va_arg(args, uint32_t);
                    const char* hex = "0123456789abcdef";
                    if (i < size - 1) str[i++] = '0';
                    if (i < size - 1) str[i++] = 'x';
                    
                    for (int j = 28; j >= 0 && i < size - 1; j -= 4) {
                        char c = hex[(val >> j) & 0xf];
                        str[i++] = c;
                    }
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) {
                        s = "(null)";
                    }
                    
                    while (*s && i < size - 1) {
                        str[i++] = *s++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    if (i < size - 1) str[i++] = c;
                    break;
                }
                default: {
                    if (i < size - 1) str[i++] = *format;
                    break;
                }
            }
        } else {
            if (i < size - 1) str[i++] = *format;
        }
        format++;
    }
    
    str[i] = '\0';
    
    va_end(args);
    
    return i;
}
