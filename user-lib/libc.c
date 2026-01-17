#include <stddef.h>
#include <syscall.h>

// 系统调用封装函数
static int syscall(int num, ...) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}

// 写入文件
ssize_t write(int fd, const void* buf, size_t count) {
    return syscall(SYS_write, fd, buf, count);
}

// 读取文件
ssize_t read(int fd, void* buf, size_t count) {
    return syscall(SYS_read, fd, buf, count);
}

// 打开文件
int open(const char* path, int flags) {
    return syscall(SYS_open, path, flags);
}

// 关闭文件
int close(int fd) {
    return syscall(SYS_close, fd);
}

// 调整进程堆大小
void* sbrk(intptr_t increment) {
    return (void*)syscall(SYS_sbrk, increment);
}

// 进程睡眠
int sleep(unsigned int seconds) {
    return syscall(SYS_sleep, seconds);
}

// 简单的malloc实现，使用sbrk
void* malloc(size_t size) {
    // 每次分配4KB的倍数
    size = (size + 4095) & ~4095;
    return sbrk(size);
}

// 简单的free实现（目前不实际释放内存，仅返回成功）
void free(void* ptr) {
    // 简化实现：不释放内存，仅返回
    return;
}

// 格式化输出函数
int printf(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    int i = 0;
    
    va_start(args, format);
    
    // 简单的格式化输出实现
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
                    for (int k = 0; k < j && i < 1023; k++) {
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
                    
                    while (*str && i < 1023) {
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
    
    va_end(args);
    
    // 写入到标准输出
    int ret = write(1, buffer, i);
    
    // 同时写入到日志服务
    int log_fd = open("/dev/log", O_WRONLY);
    if (log_fd >= 0) {
        write(log_fd, buffer, i);
        close(log_fd);
    }
    
    return ret;
}

// 进程退出
void exit(int status) {
    syscall(SYS_exit, status);
}

