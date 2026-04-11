#include <shell.h>
#include <loader/elf.h>
#include <keyboard.h>
#include <vga.h>
#include <string.h>
#include <common.h>
#include <serial.h>
#include <proc/task.h>
#include <mm/paging.h>

/* icm_execve — loads an ELF and jumps to it in user mode via iretq.
   The trampoline RSP returned by elf_load points to a kernel-stack frame:
     [rsp+0 ] = user RIP
     [rsp+8 ] = user CS  (0x1B, RPL=3)
     [rsp+16] = user RFLAGS
     [rsp+24] = user RSP
     [rsp+32] = user SS   (0x23, RPL=3)
   iretq pops all 5 values, loads CS=0x1B, SS=0x23 → CPL=3 (user mode).
   After iretq this function never returns. */
extern int icm_execve(const char *path, char **argv, char **envp) {
    (void)argv; (void)envp;
    uint64_t trampoline_rsp = 0;
    serial_write_string("[EXEC] elf_load: ");
    serial_write_string(path);
    serial_write_string("\n");
    if (elf_load(path, &trampoline_rsp) < 0) {
        serial_write_string("[EXEC] elf_load failed\n");
        return -1;
    }
    serial_write_string("[EXEC] iretq to user mode, rsp=0x");
    serial_write_hex64(trampoline_rsp);
    serial_write_string("\n");
    __asm__ volatile(
        "mov %0, %%rsp\n\t"
        "iretq\n\t"
        : : "r"(trampoline_rsp) : "memory"
    );
    __builtin_unreachable();
    return 0;
}

static char shell_buffer[SHELL_BUFFER_SIZE];
static int shell_buffer_pos = 0;

static shell_command_t shell_commands[] = {
    {"help", shell_cmd_help, "Show available commands"},
    {"echo", shell_cmd_echo, "Echo text to screen"},
    {"clear", shell_cmd_clear, "Clear the screen"},
    {"version", shell_cmd_version, "Show version information"},
    {"ai", shell_cmd_ai, "Ask AI a question (via serial port)"},
    {"ps", shell_cmd_ps, "Show process list"},
    {"free", shell_cmd_free, "Show memory usage"},
    {"top", shell_cmd_top, "Show running processes"},
    {NULL, NULL, NULL}
};

void shell_init(void)
{
    serial_write_string("[SH] shell_init\n");
    shell_buffer_pos = 0;
    serial_write_string("[SH] before kprint\n");
    kprint("\nShell initialized. Type 'help' for available commands.\n\n");
    serial_write_string("[SH] shell_init done\n");
}

void shell_prompt(void)
{
    serial_write_string("[SH] prompt\n");
    serial_write_string("synapse> ");
}

void shell_parse_command(char* input, int* argc, char** argv)
{
    *argc = 0;
    char* token = strtok(input, " ");
    
    while (token != NULL && *argc < SHELL_MAX_ARGS) {
        argv[*argc] = token;
        (*argc)++;
        token = strtok(NULL, " ");
    }
}

void shell_cmd_help(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    kprint("\nAvailable commands:\n");
    
    for (int i = 0; shell_commands[i].name != NULL; i++) {
        kprint("  ");
        kprint(shell_commands[i].name);
        kprint(" - ");
        kprint(shell_commands[i].description);
        kprint("\n");
    }
}

void shell_cmd_echo(int argc, char** argv)
{
    if (argc < 2) {
        kprint("\n");
        return;
    }
    
    kprint("\n");
    for (int i = 1; i < argc; i++) {
        kprint(argv[i]);
        if (i < argc - 1) {
            kprint(" ");
        }
    }
    kprint("\n");
}

void shell_cmd_clear(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    vga_clear();
}

void shell_cmd_version(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    kprint("\nSynapse OS v");
    kprint(KERNEL_VERSION);
    kprint("\n");
    kprint("AI-Native Operating System\n");
    kprint("Built: " __DATE__ " " __TIME__ "\n");
}

void shell_cmd_ai(int argc, char** argv)
{
    if (argc < 2) {
        kprint("\nUsage: ai <question>\n");
        kprint("Ask AI a question via serial port.\n");
        kprint("Note: Make sure the AI proxy script is running on the host.\n");
        return;
    }
    
    kprint("\n");
    kprint("Sending question to AI...\n");
    
    serial_write_string("AI_QUESTION:");
    
    for (int i = 1; i < argc; i++) {
        serial_write_string(argv[i]);
        if (i < argc - 1) {
            serial_write_char(' ');
        }
    }
    
    serial_write_char('\n');
    
    kprint("Waiting for AI response...\n");
    
    int timeout = 0;
    const int max_timeout = 100000;
    
    while (timeout < max_timeout) {
        if (serial_received()) {
            kprint("\nAI Response:\n");
            
            char c;
            while (1) {
                if (serial_can_read()) {
                    c = serial_read_char();
                    
                    if (c == '\n' || c == '\r') {
                        kprint("\n");
                        break;
                    }
                    
                    vga_putc(c);
                } else {
                    timeout++;
                    if (timeout >= max_timeout) {
                        kprint("\n");
                        break;
                    }
                }
            }
            
            return;
        }
        timeout++;
    }
    
    kprint("Timeout: No response from AI.\n");
}

// 显示进程列表
void shell_cmd_ps(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    kprint("\nPID  STATE  NAME       PRIORITY\n");

    task_t* t = current_task;
    if (t) {
        const char* state_str = "RUNNING";
        char buf[64];
        snprintf(buf, sizeof(buf), "%d    %s  %s  %d\n",
                 t->pid, state_str, t->name, t->priority);
        kprint(buf);
    }
    kprint("\n");
}

// 显示内存使用情况
void shell_cmd_free(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    kprint("\nMemory Usage:\n");
    
    size_t total = get_total_memory();
    size_t used = get_used_memory();
    size_t free = get_free_memory();
    
    char buf[128];
    snprintf(buf, sizeof(buf), "Total: %d KB\nUsed: %d KB\nFree: %d KB\n", 
              total / 1024, used / 1024, free / 1024);
    kprint(buf);
    
    size_t kheap_total, kheap_used, kheap_free;
    get_kheap_info(&kheap_total, &kheap_used, &kheap_free);
    
    snprintf(buf, sizeof(buf), "Kernel Heap:\nTotal: %d KB\nUsed: %d KB\nFree: %d KB\n", 
              kheap_total / 1024, kheap_used / 1024, kheap_free / 1024);
    kprint(buf);
    
    kprint("\n");
}

// 显示正在运行的进程
void shell_cmd_top(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    kprint("\nRunning Processes:\n");
    kprint("PID  STATE  NAME       PRIORITY  RUNTIME\n");

    task_t* t = current_task;
    if (t) {
        const char* state_str = "RUNNING";
        char buf[64];
        snprintf(buf, sizeof(buf), "%d    %s  %s  %d  %d\n",
                 t->pid, state_str, t->name, t->priority, t->total_runtime);
        kprint(buf);
    }
    kprint("\n");
}

void shell_run(void)
{
    serial_write_string("[SH] shell_run entered\n");
    char* argv[SHELL_MAX_ARGS];
    int argc;
    
    while (1) {
        shell_prompt();
        
        shell_buffer_pos = 0;
        shell_buffer[0] = '\0';
        
        /* 刷清串口接收缓冲区残留 */
        {
            extern bool serial_can_read(void);
            extern char serial_read_char(void);
            while (serial_can_read()) serial_read_char();
        }
        while (1) {
            serial_write_string("[SH] waiting\n");
            char c = serial_read_char();
            serial_write_string("[SH] got=");
            serial_write_hex((uint32_t)(uint8_t)c);
            serial_write_string("\n");
            if (c == '\r' || c == '\n') {
                serial_write_string("\r\n");
                break;
            } else if (c == '\b' || c == 127) {
                if (shell_buffer_pos > 0) {
                    shell_buffer_pos--;
                    shell_buffer[shell_buffer_pos] = '\0';
                    serial_write_string("\b \b");
                }
            } else if (c >= 32 && c <= 126) {
                if (shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
                    shell_buffer[shell_buffer_pos] = c;
                    shell_buffer_pos++;
                    shell_buffer[shell_buffer_pos] = '\0';
                    serial_write_char(c);
                }
            }
        }
        
        if (shell_buffer_pos == 0) {
            serial_write_string("[SH] empty line, continue\n");
            continue;
        }
        
        shell_parse_command(shell_buffer, &argc, argv);
        
        if (argc > 0) {
            bool found = false;
            
            for (int i = 0; shell_commands[i].name != NULL; i++) {
                if (strcmp(argv[0], shell_commands[i].name) == 0) {
                    shell_commands[i].func(argc, argv);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                kprint("Executing: ");
                kprint(argv[0]);
                kprint("\n");
                
                int ret = icm_execve(argv[0], argv, NULL);
                if (ret < 0) {
                    kprint("Failed to execute: ");
                    kprint(argv[0]);
                    kprint("\n");
                }
            }
        }
    }
}