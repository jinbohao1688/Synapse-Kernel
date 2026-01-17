#include <proc/task.h>
#include <proc/sched.h>
#include <vga.h>
#include <serial.h>
#include <string.h>

// 安全操作执行器

// 操作类型枚举
typedef enum {
    AI_ACTION_ALERT,
    AI_ACTION_KILL,
    AI_ACTION_PAUSE,
    AI_ACTION_RESUME
} ai_action_type_t;

// 操作结构体
typedef struct {
    ai_action_type_t type;
    uint32_t pid;
    char reason[256];
    char message[256];
} ai_action_t;

// 简单的JSON解析（简化实现）
static int parse_ai_action(const char* json, ai_action_t* action) {
    // 简化实现：仅解析基本的操作类型和PID
    // 实际应用中应使用完整的JSON解析库
    
    // 检查操作类型
    if (strstr(json, "\"type\": \"kill\"")) {
        action->type = AI_ACTION_KILL;
    } else if (strstr(json, "\"type\": \"pause\"")) {
        action->type = AI_ACTION_PAUSE;
    } else if (strstr(json, "\"type\": \"resume\"")) {
        action->type = AI_ACTION_RESUME;
    } else if (strstr(json, "\"type\": \"alert\"")) {
        action->type = AI_ACTION_ALERT;
    } else {
        return -1;
    }
    
    // 提取PID（简化实现）
    char* pid_str = strstr(json, "\"pid\":");
    if (pid_str && action->type != AI_ACTION_ALERT) {
        pid_str += 7; // 跳过 "pid":
        while (*pid_str == ' ' || *pid_str == '\t' || *pid_str == ':') {
            pid_str++;
        }
        action->pid = atoi(pid_str);
    }
    
    // 提取原因或消息
    if (action->type == AI_ACTION_ALERT) {
        char* msg_str = strstr(json, "\"message\":");
        if (msg_str) {
            msg_str += 11; // 跳过 "message":
            while (*msg_str == ' ' || *msg_str == '\t' || *msg_str == '"') {
                msg_str++;
            }
            
            // 复制消息内容，直到遇到下一个引号
            int i = 0;
            while (*msg_str && *msg_str != '"' && i < sizeof(action->message) - 1) {
                action->message[i++] = *msg_str++;
            }
            action->message[i] = '\0';
        }
    } else {
        char* reason_str = strstr(json, "\"reason\":");
        if (reason_str) {
            reason_str += 10; // 跳过 "reason":
            while (*reason_str == ' ' || *reason_str == '\t' || *reason_str == '"') {
                reason_str++;
            }
            
            // 复制原因内容，直到遇到下一个引号
            int i = 0;
            while (*reason_str && *reason_str != '"' && i < sizeof(action->reason) - 1) {
                action->reason[i++] = *reason_str++;
            }
            action->reason[i] = '\0';
        }
    }
    
    return 0;
}

// 执行AI建议
void execute_ai_action(const char* json_action) {
    ai_action_t action;
    memset(&action, 0, sizeof(action));
    
    // 解析JSON操作
    if (parse_ai_action(json_action, &action) < 0) {
        kprintf("[AI_EXECUTOR] Failed to parse AI action: %s\n", json_action);
        return;
    }
    
    // 根据操作类型执行相应操作
    switch (action.type) {
        case AI_ACTION_ALERT:
            // 仅显示警告消息
            kprintf("[AI_ALERT] %s\n", action.message);
            break;
            
        case AI_ACTION_KILL:
            // 终止进程，需要用户确认
            kprintf("[AI建议]：终止进程 %d？原因：%s (y/N): ", action.pid, action.reason);
            
            // 等待用户输入
            char c = vga_getc();
            vga_putc(c);
            vga_putc('\n');
            
            if (c == 'y' || c == 'Y') {
                // 查找并终止进程
                task_t* task = current_task;
                
                // 检查当前进程
                if (task->pid == action.pid) {
                    // 终止当前进程
                    task_exit(0);
                    return;
                }
                
                // 遍历就绪队列查找目标进程
                extern task_t* ready_queue[];
                for (int i = 0; i < MAX_PRIORITY; i++) {
                    task_t* t = ready_queue[i];
                    while (t) {
                        if (t->pid == action.pid) {
                            // 设置进程状态为僵尸，等待调度器清理
                            t->state = TASK_ZOMBIE;
                            kprintf("[AI_EXECUTOR] Terminated process %d\n", action.pid);
                            return;
                        }
                        t = t->sibling_next;
                    }
                }
                
                kprintf("[AI_EXECUTOR] Process %d not found\n", action.pid);
            } else {
                kprintf("[AI_EXECUTOR] Action canceled by user\n");
            }
            break;
            
        case AI_ACTION_PAUSE:
            // 暂停进程，需要用户确认
            kprintf("[AI建议]：暂停进程 %d？原因：%s (y/N): ", action.pid, action.reason);
            
            // 等待用户输入
            c = vga_getc();
            vga_putc(c);
            vga_putc('\n');
            
            if (c == 'y' || c == 'Y') {
                // 查找并暂停进程
                extern task_t* ready_queue[];
                for (int i = 0; i < MAX_PRIORITY; i++) {
                    task_t* t = ready_queue[i];
                    while (t) {
                        if (t->pid == action.pid) {
                            // 将进程状态改为阻塞
                            t->state = TASK_BLOCKED;
                            kprintf("[AI_EXECUTOR] Paused process %d\n", action.pid);
                            return;
                        }
                        t = t->sibling_next;
                    }
                }
                
                kprintf("[AI_EXECUTOR] Process %d not found\n", action.pid);
            } else {
                kprintf("[AI_EXECUTOR] Action canceled by user\n");
            }
            break;
            
        case AI_ACTION_RESUME:
            // 恢复进程，需要用户确认
            kprintf("[AI建议]：恢复进程 %d？原因：%s (y/N): ", action.pid, action.reason);
            
            // 等待用户输入
            c = vga_getc();
            vga_putc(c);
            vga_putc('\n');
            
            if (c == 'y' || c == 'Y') {
                // 查找并恢复进程
                extern task_t* ready_queue[];
                for (int i = 0; i < MAX_PRIORITY; i++) {
                    task_t* t = ready_queue[i];
                    while (t) {
                        if (t->pid == action.pid) {
                            // 将进程状态改为就绪
                            t->state = TASK_READY;
                            kprintf("[AI_EXECUTOR] Resumed process %d\n", action.pid);
                            return;
                        }
                        t = t->sibling_next;
                    }
                }
                
                kprintf("[AI_EXECUTOR] Process %d not found\n", action.pid);
            } else {
                kprintf("[AI_EXECUTOR] Action canceled by user\n");
            }
            break;
            
        default:
            kprintf("[AI_EXECUTOR] Unknown action type\n");
            break;
    }
}

// 初始化AI执行器
void ai_executor_init(void) {
    kprintf("[AI_EXECUTOR] AI executor initialized\n");
}
