/*
 * icm_shell.c — ICM-OS Shell for Synapse Kernel
 * 纯 freestanding，仅依赖 icm_lib.h / syscall_fix.h
 */
#include "icm_lib.h"
#include "syscall_fix.h"

#define BUFSIZE  512
#define ARGBUF   16   /* 最多参数个数 */

/* ── 辅助 ─────────────────────────────────────── */

static void print_nl(void) { icm_puts("\n"); }

static void print_str(const char *s) { if (s) icm_puts(s); }

/* 跳过开头的空格 / tab */
static char *skip_ws(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/* 把 src 前 n 字符复制到 dst 并以 NUL 结尾 */
static void copyn(char *dst, const char *src, int n) {
    int i;
    for (i = 0; i < n && src[i]; i++) dst[i] = src[i];
    dst[i] = '\0';
}

/* ── help ─────────────────────────────────────── */

static void cmd_help(void) {
    icm_puts("ICM-OS Shell commands:\n");
    icm_puts("  help             - show this help\n");
    icm_puts("  version          - show version\n");
    icm_puts("  exit             - exit shell\n");
    icm_puts("  read <path>      - read and print file contents\n");
    icm_puts("  write <path> <text> - create/truncate file with text\n");
    icm_puts("  !<cmd> [args...] - fork+exec program, search /bin and /usr/bin\n");
    print_nl();
}

/* ── version ─────────────────────────────────── */

static void cmd_version(void) {
    icm_puts("ICM-OS Shell for Synapse Kernel\n");
    icm_puts("Version 0.1 (freestanding, no libc)\n");
    print_nl();
}

/* ── read ────────────────────────────────────── */

static void cmd_read(char *path) {
    char buf[BUFSIZE];
    int fd;
    int n;

    path = skip_ws(path);
    if (!*path) {
        icm_puts("read: missing path\n");
        return;
    }

    fd = sys_open(path, O_RDONLY);
    if (fd < 0) {
        icm_puts("read: cannot open '");
        icm_puts(path);
        icm_puts("'\n");
        return;
    }

    for (;;) {
        n = sys_read(fd, buf, BUFSIZE - 1);
        if (n <= 0) break;
        sys_write(1, buf, n);
    }
    sys_close(fd);
}

/* ── write ───────────────────────────────────── */

/* 找到命令行中第一个非空格字符后，提取 path 和 content */
static void cmd_write(char *line) {
    char *path;
    char *text;
    int fd;
    int len;

    /* line 格式: write <path> <text> */
    line = skip_ws(line);
    /* 跳过 "write" */
    while (*line && *line != ' ' && *line != '\t') line++;
    line = skip_ws(line);
    if (!*line) {
        icm_puts("write: usage: write <path> <text>\n");
        return;
    }

    path = line;
    /* 找到第一个空格（path 结束） */
    while (*line && *line != ' ' && *line != '\t') line++;
    if (!*line) {
        icm_puts("write: missing text content\n");
        return;
    }
    *line++ = '\0';
    text = skip_ws(line);
    if (!*text) {
        icm_puts("write: missing text content\n");
        return;
    }

    fd = sys_open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        icm_puts("write: cannot create '");
        icm_puts(path);
        icm_puts("'\n");
        return;
    }
    len = icm_strlen(text);
    sys_write(fd, text, len);
    sys_close(fd);

    icm_puts("write: wrote ");
    icm_puts(path);
    icm_puts("\n");
}

/* ── exec ─────────────────────────────────────── */

static char *_search_path(const char *name, char *buf, int bufsz) {
    static const char *dirs[] = { "/bin/", "/usr/bin/" };
    int d, i;
    int len;

    for (d = 0; d < 2; d++) {
        icm_memset(buf, 0, bufsz);
        len = icm_strlen(dirs[d]);
        if (len + icm_strlen(name) + 1 > bufsz) continue;
        for (i = 0; dirs[d][i]; i++) buf[i] = dirs[d][i];
        for (i = 0; name[i]; i++) buf[len + i] = name[i];
        buf[len + i] = '\0';
        /* open 只检查文件是否存在 */
        if (sys_open(buf, O_RDONLY) >= 0) {
            return buf;
        }
    }
    return 0;
}

static void cmd_exec(char *cmdline) {
    char argbuf[ARGBUF][128];     /* 参数缓冲区 */
    char *argv[ARGBUF + 2];
    char binpath[256];
    int argc = 0;
    int pid;
    int status;
    char *p;

    cmdline = skip_ws(cmdline);
    if (!*cmdline) {
        icm_puts("!: empty command\n");
        return;
    }

    /* 解析参数（简单空格分割） */
    p = cmdline;
    while (*p && argc < ARGBUF) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        /* 记录本参数起始 */
        argv[argc] = argbuf[argc];
        /* 复制到 argbuf[argc]，截断超长参数 */
        {
            int i = 0;
            while (*p && *p != ' ' && *p != '\t' && i < 126) {
                argbuf[argc][i++] = *p++;
            }
            argbuf[argc][i] = '\0';
        }
        if (!*argbuf[argc]) break;
        argc++;
    }
    argv[argc] = 0; /* NULL terminate */

    if (argc == 0) {
        icm_puts("!: empty args\n");
        return;
    }

    /* 搜索 bin */
    if (!_search_path(argv[0], binpath, sizeof binpath)) {
        icm_puts("!: command not found: ");
        icm_puts(argv[0]);
        print_nl();
        return;
    }

    /* fork */
    pid = icm_fork();
    if (pid < 0) {
        icm_puts("!: fork failed\n");
        return;
    }
    if (pid == 0) {
        /* child */
        icm_execve(binpath, argv, 0);
        /* execve 返回说明失败了 */
        icm_puts("!: exec failed: ");
        icm_puts(binpath);
        print_nl();
        icm_exit(1);
        /* not reached */
    }
    /* parent: wait */
    status = 0;
    icm_wait(&status);
    icm_puts("!: exit status ");
    icm_write_int(status & 0xff);
    print_nl();
}

/* ── 交互循环 ─────────────────────────────────── */

static void prompt(void) {
    icm_puts("icm> ");
}

int main(void) {
    char line[BUFSIZE];

    icm_puts("\n");
    icm_puts("  ICM-OS Shell [Synapse Kernel]\n");
    icm_puts("  Type 'help' for commands, 'exit' to quit.\n");
    print_nl();

    for (;;) {
        prompt();
        if (icm_getline(line, BUFSIZE) <= 0) {
            /* EOF */
            icm_puts("\nexit\n");
            break;
        }

        /* 空行直接跳过 */
        if (!*skip_ws(line)) continue;

        /* help */
        if (icm_strcmp(line, "help") == 0 || icm_strcmp(line, "?") == 0) {
            cmd_help();
            continue;
        }
        /* version */
        if (icm_strcmp(line, "version") == 0) {
            cmd_version();
            continue;
        }
        /* exit */
        if (icm_strcmp(line, "exit") == 0 || icm_strcmp(line, "quit") == 0) {
            icm_puts("goodbye.\n");
            break;
        }
        /* read */
        if (icm_strncmp(line, "read ", 5) == 0) {
            cmd_read(line + 5);
            continue;
        }
        /* write */
        if (icm_strncmp(line, "write ", 6) == 0) {
            cmd_write(line);
            continue;
        }
        /* exec: ! */
        if (line[0] == '!') {
            cmd_exec(line + 1);
            continue;
        }

        /* 未知 */
        icm_puts("unknown: ");
        icm_puts(line);
        print_nl();
    }

    icm_exit(0);
    return 0; /* unreachable */
}
