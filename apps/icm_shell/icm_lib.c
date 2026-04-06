/*
 * icm_lib.c — freestanding 实现
 */
#include "syscall_fix.h"
#include "icm_lib.h"

#define STDIN  0
#define STDOUT 1
#define STDERR 2

/* ── 字符串 ─────────────────────────────────────── */

int icm_strlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

char *icm_strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
    return dst;
}

char *icm_strncpy(char *dst, const char *src, int n) {
    int i;
    for (i = 0; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

char *icm_strcat(char *dst, const char *src) {
    int d = icm_strlen(dst);
    int i = 0;
    while (src[i]) { dst[d + i] = src[i]; i++; }
    dst[d + i] = '\0';
    return dst;
}

int icm_strcmp(const char *a, const char *b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return a[i] - b[i];
}

int icm_strncmp(const char *a, const char *b, int n) {
    int i = 0;
    while (i < n && a[i] && b[i]) {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return (i < n) ? (a[i] - b[i]) : 0;
}

char *icm_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return 0;
}

int icm_atoi(const char *s) {
    int neg = 0;
    int v = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return neg ? -v : v;
}

/* ── 内存 ───────────────────────────────────────── */

void *icm_memset(void *dst, int val, int n) {
    char *p = (char *)dst;
    int i;
    for (i = 0; i < n; i++) p[i] = (char)val;
    return dst;
}

void *icm_memcpy(void *dst, const void *src, int n) {
    char *d = (char *)dst;
    const char *s = (const char *)src;
    int i;
    for (i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

/* ── IO ─────────────────────────────────────────── */

void icm_putchar(int c) {
    char ch = (char)c;
    sys_write(STDOUT, &ch, 1);
}

void icm_puts(const char *s) {
    sys_write(STDOUT, s, icm_strlen(s));
}

int icm_getline(char *buf, int max) {
    int n = 0;
    char ch;
    int r;
    if (max <= 0) return 0;
    max--; /* reserve space for NUL */
    while (n < max) {
        r = sys_read(STDIN, &ch, 1);
        if (r <= 0) break; /* EOF or error */
        if (ch == '\n') {
            buf[n] = '\0';
            return n;
        }
        if (ch == '\r') {
            /* treat \r\n or lone \r as end */
            buf[n] = '\0';
            return n;
        }
        buf[n++] = ch;
    }
    buf[n] = '\0';
    return n;
}

/* 简单整数 → 字符串打印（无符号） */
static void _putu(unsigned int v) {
    char tmp[12];
    int i = 0;
    if (v == 0) { icm_putchar('0'); return; }
    while (v > 0) {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i--) icm_putchar(tmp[i]);
}

void icm_write_int(int v) {
    if (v < 0) { icm_putchar('-'); v = (unsigned int)(-(v + 1)) + 1U; }
    _putu((unsigned int)v);
}

/* ── 进程 ───────────────────────────────────────── */

void icm_exit(int code) { sys_exit(code & 0xff); }
int  icm_fork(void)      { return sys_fork(); }
int  icm_wait(int *s)    { return sys_wait(s); }

int icm_execve(const char *path, char **argv, char **envp) {
    return sys_execve(path, argv, envp);
}
