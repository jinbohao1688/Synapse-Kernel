#include <shell.h>
#include <keyboard.h>
#include <vga.h>
#include <string.h>
#include <common.h>
#include <serial.h>
#include <proc/task.h>
#include <mm/paging.h>

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
    shell_buffer_pos = 0;
    kprint("\nShell initialized. Type 'help' for available commands.\n\n");
}

void shell_prompt(void)
{
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprint("synapse> ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
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
    
    kprint("\nPID\tSTATE\tNAME\tPRIORITY\tTIME\t\n");
    
    // 遍历所有进程
    struct task* task = task_list;
    if (task) {
        do {
            // 获取进程状态字符串
            const char* state_str;
            switch (task->state) {
                case TASK_READY: state_str = "READY";
                    break;
                case TASK_RUNNING: state_str = "RUNNING";
                    break;
                case TASK_BLOCKED: state_str = "BLOCKED";
                    break;
                case TASK_ZOMBIE: state_str = "ZOMBIE";
                    break;
                case TASK_EXITED: state_str = "EXITED";
                    break;
                default: state_str = "UNKNOWN";
                    break;
            }
            
            // 输出进程信息
            char buf[128];
            snprintf(buf, sizeof(buf), "%d\t%s\tTask%d\t%d\t%d\t\n", 
                      task->pid, state_str, task->pid, task->priority, task->total_time);
            kprint(buf);
            
            task = task->next;
        } while (task != task_list);
    }
    
    kprint("\n");
}

// 显示内存使用情况
void shell_cmd_free(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    kprint("\nMemory Usage:\n");
    
    // 获取内存信息
    size_t total = get_total_memory();
    size_t used = get_used_memory();
    size_t free = get_free_memory();
    
    char buf[128];
    snprintf(buf, sizeof(buf), "Total: %d KB\nUsed: %d KB\nFree: %d KB\n", 
              total / 1024, used / 1024, free / 1024);
    kprint(buf);
    
    // 获取内核堆信息
    size_t kheap_total, kheap_used, kheap_free;
    get_kheap_info(&kheap_total, &kheap_used, &kheap_free);
    
    snprintf(buf, sizeof(buf), "Kernel Heap:\nTotal: %d KB\nUsed: %d KB\nFree: %d KB\n", 
              kheap_total / 1024, kheap_used / 1024, kheap_free / 1024);
    kprint(buf);
    
    kprint("\n");
}

// 显示正在运行的进程（简化版top命令）
void shell_cmd_top(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    kprint("\nRunning Processes:\n");
    kprint("PID\tSTATE\tNAME\tPRIORITY\tTIME\t\n");
    
    // 遍历所有进程，只显示运行中的进程
    struct task* task = task_list;
    if (task) {
        do {
            // 只显示运行中和就绪状态的进程
            if (task->state == TASK_RUNNING || task->state == TASK_READY) {
                // 获取进程状态字符串
                const char* state_str;
                switch (task->state) {
                    case TASK_READY: state_str = "READY";
                        break;
                    case TASK_RUNNING: state_str = "RUNNING";
                        break;
                    default: state_str = "UNKNOWN";
                        break;
                }
                
                // 输出进程信息
                char buf[128];
                snprintf(buf, sizeof(buf), "%d\t%s\tTask%d\t%d\t%d\t\n", 
                          task->pid, state_str, task->pid, task->priority, task->total_time);
                kprint(buf);
            }
            
            task = task->next;
        } while (task != task_list);
    }
    
    kprint("\n");
}

void shell_run(void)
{
    char* argv[SHELL_MAX_ARGS];
    int argc;
    
    while (1) {
        shell_prompt();
        
        shell_buffer_pos = 0;
        shell_buffer[0] = '\0';
        
        while (1) {
            key_event_t event;
            
            if (keyboard_read(&event) && event.pressed) {
                if (event.ascii == '\n') {
                    kprint("\n");
                    break;
                } else if (event.ascii == '\b') {
                    if (shell_buffer_pos > 0) {
                        shell_buffer_pos--;
                        shell_buffer[shell_buffer_pos] = '\0';
                        vga_putc('\b');
                    }
                } else if (event.ascii >= 32 && event.ascii <= 126) {
                    if (shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
                        shell_buffer[shell_buffer_pos] = event.ascii;
                        shell_buffer_pos++;
                        shell_buffer[shell_buffer_pos] = '\0';
                        vga_putc(event.ascii);
                    }
                }
            }
        }
        
        if (shell_buffer_pos == 0) {
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
                // 尝试作为可执行文件执行
                kprint("Executing: ");
                kprint(argv[0]);
                kprint("\n");
                
                // 使用sys_execve执行文件
                int ret = syscall(SYS_execve, argv[0], argv, NULL);
                if (ret < 0) {
                    kprint("Failed to execute: ");
                    kprint(argv[0]);
                    kprint("\n");
                }
            }
        }
    }
}
