/*
 * icm_lib.h — freestanding 字符串/IO 库，不依赖任何系统头文件
 */
#ifndef ICM_LIB_H
#define ICM_LIB_H

/* ── 字符串 ─────────────────────────────────────── */
int   icm_strlen(const char *s);
int   icm_strcmp(const char *a, const char *b);
int   icm_strncmp(const char *a, const char *b, int n);
char *icm_strcpy(char *dst, const char *src);
char *icm_strcat(char *dst, const char *src);
char *icm_strncpy(char *dst, const char *src, int n);
char *icm_strchr(const char *s, int c);
int   icm_atoi(const char *s);

/* ── 内存 ───────────────────────────────────────── */
void *icm_memset(void *dst, int val, int n);
void *icm_memcpy(void *dst, const void *src, int n);

/* ── IO ─────────────────────────────────────────── */
void  icm_puts(const char *s);          /* sys_write(STDOUT) */
void  icm_putchar(int c);               /* 写入一个字符 */
int   icm_getline(char *buf, int max);  /* 从 STDIN 读一行，返回长度（不含 \n） */
void  icm_write_int(int v);             /* 整数输出 */

/* ── 工具 ───────────────────────────────────────── */
void  icm_exit(int code);              /* sys_exit */
int   icm_fork(void);                  /* sys_fork */
int   icm_wait(int *status);           /* sys_wait */
int   icm_execve(const char*path,char**argv,char**envp); /* sys_execve */

#endif /* ICM_LIB_H */
