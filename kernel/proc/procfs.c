#include <fs.h>
#include <proc/task.h>
#include <string.h>
#include <vga.h>
#include <mm/kheap.h>

// 系统负载数据
static uint32_t load_avg[3] = {0, 0, 0}; // 1, 5, 15分钟负载
static uint32_t load_ticks[3] = {0, 0, 0}; // 负载计数
static uint32_t last_tick = 0; // 上次更新时间

// 生成系统负载内容
static int generate_proc_loadavg_content(char* buf, size_t buf_size) {
    if (!buf) {
        return 0;
    }
    
    // 简化实现：返回固定负载值
    int offset = snprintf(buf, buf_size, "0.10 0.20 0.30 1/1 %d\n", current_task ? current_task->pid : 0);
    
    return offset;
}

// 生成内存信息内容
static int generate_proc_meminfo_content(char* buf, size_t buf_size) {
    if (!buf) {
        return 0;
    }
    
    // 获取内存使用信息（简化实现）
    uint32_t total_mem = 32 * 1024 * 1024; // 假设32MB总内存
    uint32_t free_mem = get_free_mem_size();
    uint32_t used_mem = total_mem - free_mem;
    
    // 输出内存信息
    int offset = snprintf(buf, buf_size, "MemTotal:     %d kB\n", total_mem / 1024);
    offset += snprintf(buf + offset, buf_size - offset, "MemFree:      %d kB\n", free_mem / 1024);
    offset += snprintf(buf + offset, buf_size - offset, "MemUsed:      %d kB\n", used_mem / 1024);
    offset += snprintf(buf + offset, buf_size - offset, "\nBuddy Allocator Fragmentation:\n");
    offset += snprintf(buf + offset, buf_size - offset, "  Level 0: 10%%\n");
    offset += snprintf(buf + offset, buf_size - offset, "  Level 1: 15%%\n");
    offset += snprintf(buf + offset, buf_size - offset, "  Level 2: 8%%\n");
    offset += snprintf(buf + offset, buf_size - offset, "  Level 3: 5%%\n");
    
    return offset;
}

// 生成进程文件描述符内容
static int generate_proc_pid_fd_content(uint32_t pid, char* buf, size_t buf_size) {
    if (!buf) {
        return 0;
    }
    
    // 简化实现：返回固定的文件描述符列表
    int offset = snprintf(buf, buf_size, "0\n1\n2\n");
    
    return offset;
}

// 生成进程列表内容
static int generate_proc_ps_content(char* buf, size_t buf_size) {
    if (!buf) {
        return 0;
    }
    
    // 输出表头
    int offset = snprintf(buf, buf_size, "PID\tSTATE\tNAME\tPRIORITY\tTIME\t\n");
    if (offset >= buf_size) {
        return offset;
    }
    
    // 获取当前进程
    extern task_t* current_task;
    
    // 输出当前进程信息
    if (current_task) {
        // 获取进程状态字符串
        const char* state_str;
        switch (current_task->state) {
            case TASK_READY: state_str = "READY";
                break;
            case TASK_RUNNING: state_str = "RUNNING";
                break;
            case TASK_BLOCKED: state_str = "BLOCKED";
                break;
            case TASK_ZOMBIE: state_str = "ZOMBIE";
                break;
            default: state_str = "UNKNOWN";
                break;
        }
        
        // 输出进程信息
        offset += snprintf(buf + offset, buf_size - offset, "%d\t%s\t%s\t%d\t%d\t\n", 
                          current_task->pid, state_str, current_task->name, current_task->priority, current_task->total_runtime);
        
        if (offset >= buf_size) {
            return offset;
        }
    }
    
    // 遍历所有就绪队列
    extern task_t* ready_queue[];
    for (int i = 0; i < MAX_PRIORITY; i++) {
        task_t* task = ready_queue[i];
        while (task) {
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
                default: state_str = "UNKNOWN";
                    break;
            }
            
            // 输出进程信息
            offset += snprintf(buf + offset, buf_size - offset, "%d\t%s\t%s\t%d\t%d\t\n", 
                              task->pid, state_str, task->name, task->priority, task->total_runtime);
            
            if (offset >= buf_size) {
                return offset;
            }
            
            task = task->sibling_next;
        }
    }
    
    return offset;
}

// 生成单个进程状态内容
static int generate_proc_pid_status_content(uint32_t pid, char* buf, size_t buf_size) {
    if (!buf) {
        return 0;
    }
    
    task_t* task = NULL;
    
    // 首先检查当前进程
    extern task_t* current_task;
    if (current_task && current_task->pid == pid) {
        task = current_task;
    }
    
    // 如果不是当前进程，遍历就绪队列
    if (!task) {
        extern task_t* ready_queue[];
        for (int i = 0; i < MAX_PRIORITY; i++) {
            task_t* t = ready_queue[i];
            while (t) {
                if (t->pid == pid) {
                    task = t;
                    break;
                }
                t = t->sibling_next;
            }
            if (task) {
                break;
            }
        }
    }
    
    // 如果找到了进程
    if (task) {
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
            default: state_str = "UNKNOWN";
                break;
        }
        
        // 输出进程详细信息
        int offset = snprintf(buf, buf_size, "Process Information\n");
        offset += snprintf(buf + offset, buf_size - offset, "PID: %d\n", task->pid);
        offset += snprintf(buf + offset, buf_size - offset, "State: %s\n", state_str);
        offset += snprintf(buf + offset, buf_size - offset, "Priority: %d\n", task->priority);
        offset += snprintf(buf + offset, buf_size - offset, "Total Time: %d\n", task->total_runtime);
        offset += snprintf(buf + offset, buf_size - offset, "Parent PID: %d\n", task->parent ? task->parent->pid : 0);
        offset += snprintf(buf + offset, buf_size - offset, "Page Directory: 0x%x\n", (uint32_t)task->page_dir);
        offset += snprintf(buf + offset, buf_size - offset, "Kernel ESP: 0x%x\n", task->kernel_stack_top);
        offset += snprintf(buf + offset, buf_size - offset, "User ESP: 0x%x\n", task->user_stack_top);
        
        return offset;
    }
    
    return snprintf(buf, buf_size, "Process %d not found\n", pid);
}

// 读取/proc/ps文件
static int proc_read_ps(inode_t* inode, void* buf, size_t count, uint32_t offset) {
    char proc_buf[1024];
    
    // 生成进程列表内容
    int content_size = generate_proc_ps_content(proc_buf, sizeof(proc_buf));
    
    // 检查偏移量
    if (offset >= content_size) {
        return 0;
    }
    
    // 计算实际读取的字节数
    size_t read_size = (offset + count > content_size) ? (content_size - offset) : count;
    
    // 复制内容到缓冲区
    memcpy(buf, proc_buf + offset, read_size);
    
    return read_size;
}

// 读取/proc/[pid]/status文件
static int proc_read_pid_status(inode_t* inode, void* buf, size_t count, uint32_t offset) {
    char proc_buf[1024];
    
    // 从inode中获取PID（简化实现：假设inode->inode字段存储PID）
    uint32_t pid = inode->inode;
    
    // 生成进程状态内容
    int content_size = generate_proc_pid_status_content(pid, proc_buf, sizeof(proc_buf));
    
    // 检查偏移量
    if (offset >= content_size) {
        return 0;
    }
    
    // 计算实际读取的字节数
    size_t read_size = (offset + count > content_size) ? (content_size - offset) : count;
    
    // 复制内容到缓冲区
    memcpy(buf, proc_buf + offset, read_size);
    
    return read_size;
}

// 读取/proc/loadavg文件
static int proc_read_loadavg(inode_t* inode, void* buf, size_t count, uint32_t offset) {
    char proc_buf[128];
    
    // 生成负载内容
    int content_size = generate_proc_loadavg_content(proc_buf, sizeof(proc_buf));
    
    // 检查偏移量
    if (offset >= content_size) {
        return 0;
    }
    
    // 计算实际读取的字节数
    size_t read_size = (offset + count > content_size) ? (content_size - offset) : count;
    
    // 复制内容到缓冲区
    memcpy(buf, proc_buf + offset, read_size);
    
    return read_size;
}

// 读取/proc/meminfo文件
static int proc_read_meminfo(inode_t* inode, void* buf, size_t count, uint32_t offset) {
    char proc_buf[1024];
    
    // 生成内存信息内容
    int content_size = generate_proc_meminfo_content(proc_buf, sizeof(proc_buf));
    
    // 检查偏移量
    if (offset >= content_size) {
        return 0;
    }
    
    // 计算实际读取的字节数
    size_t read_size = (offset + count > content_size) ? (content_size - offset) : count;
    
    // 复制内容到缓冲区
    memcpy(buf, proc_buf + offset, read_size);
    
    return read_size;
}

// 读取/proc/[pid]/fd文件
static int proc_read_pid_fd(inode_t* inode, void* buf, size_t count, uint32_t offset) {
    char proc_buf[128];
    
    // 从inode中获取PID
    uint32_t pid = inode->inode;
    
    // 生成文件描述符内容
    int content_size = generate_proc_pid_fd_content(pid, proc_buf, sizeof(proc_buf));
    
    // 检查偏移量
    if (offset >= content_size) {
        return 0;
    }
    
    // 计算实际读取的字节数
    size_t read_size = (offset + count > content_size) ? (content_size - offset) : count;
    
    // 复制内容到缓冲区
    memcpy(buf, proc_buf + offset, read_size);
    
    return read_size;
}

// 初始化/proc文件系统
void init_procfs(void) {
    kprintf("[PROCFS] Proc filesystem initialized\n");
    
    // 简化实现：在实际文件系统中创建/proc目录和相关文件
    // 这里只是注册procfs的读取函数，具体的文件系统集成需要在文件系统模块中完成
}

// 获取/proc文件系统的读取函数
fs_operations_t* get_procfs_ops(void) {
    static fs_operations_t procfs_ops = {
        .read = proc_read_ps,
        // 其他操作暂时未实现
        .create = NULL,
        .open = NULL,
        .close = NULL,
        .write = NULL,
        .unlink = NULL,
        .mkdir = NULL,
        .rmdir = NULL,
        .readdir = NULL,
        .rename = NULL
    };
    
    return &procfs_ops;
}

// 获取单个进程状态的读取函数
fs_operations_t* get_proc_pid_ops(void) {
    static fs_operations_t proc_pid_ops = {
        .read = proc_read_pid_status,
        // 其他操作暂时未实现
        .create = NULL,
        .open = NULL,
        .close = NULL,
        .write = NULL,
        .unlink = NULL,
        .mkdir = NULL,
        .rmdir = NULL,
        .readdir = NULL,
        .rename = NULL
    };
    
    return &proc_pid_ops;
}
