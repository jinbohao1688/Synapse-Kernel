#include <syscall.h>
#include <proc/task.h>
#include <proc/regs.h>
#include <mm/paging.h>
#include <mm/kheap.h>
#include <string.h>
#include <vga.h>
#include <serial.h>
#include <interrupts.h>
#include <keyboard.h>
#include <loader/elf.h>
#include <fs/vfs.h>

typedef long off_t;

/* kwrite is used by sys_write_handler — writes to VGA + serial */
static void kwrite(const void* buf, size_t count)
{
    const char* p = (const char*)buf;
    for (size_t i = 0; i < count; i++) {
        vga_putc(p[i]);
        serial_write_char(p[i]);
    }
}

// System call table (extern defined in table.asm)
extern syscall_handler_t syscall_table[];

// Maximum syscall count
#define MAX_SYSCALLS 12

// x86-64 syscall: arguments in RDI, RSI, RDX (System V ABI)
/* Linux x86-64 syscall号 → Synapse syscall号 */
static uint64_t linux_to_synapse(uint64_t num) {
    switch (num) {
        case 0:  return 4;  /* Linux read  → Synapse read  */
        case 1:  return 3;  /* Linux write → Synapse write */
        case 2:  return 5;  /* Linux open  → Synapse open  */
        case 3:  return 6;  /* Linux close → Synapse close */
        case 9:  return 7;  /* Linux mmap  → Synapse mmap  */
        case 11: return 8;  /* Linux munmap→ Synapse munmap*/
        case 12: return 9;  /* Linux brk   → Synapse sbrk  */
        case 57: return 1;  /* Linux fork  → Synapse fork  */
        case 59: return 11; /* Linux execve→ Synapse execve*/
        case 60: return 0;  /* Linux exit  → Synapse exit  */
        case 61: return 2;  /* Linux wait4 → Synapse wait  */
        default: return num;
    }
}

uint64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    num = linux_to_synapse(num);
    if (num >= MAX_SYSCALLS) {
        serial_write_string("[SYSCALL] invalid: ");
        serial_write_hex64(num);
        serial_write_string("\n");
        return (uint64_t)-1;
    }
    syscall_handler_t handler = syscall_table[num];
    if (!handler) return (uint64_t)-1;

    // x86-64 SysV ABI: syscall arg1=RAX→saved in rdi slot, arg2=RSI→saved in rsi slot, arg3=RDX→saved in rdx slot
    struct regs r;
    r.rdi = arg1;
    r.rsi = arg2;
    r.rdx = arg3;
    uint64_t ret = handler(&r);
    return ret;
}

// SYS_exit — exit current process
uint64_t sys_exit_handler(struct regs* regs)
{
    (void)regs;
    task_exit(0);
    return 0;
}

// SYS_fork — create new process
uint64_t sys_fork_handler(struct regs* regs)
{
    (void)regs;
    // Simplified: fork not yet implemented in x86-64
    return (uint64_t)-1;
}

// SYS_wait — wait for child
uint64_t sys_wait_handler(struct regs* regs)
{
    (void)regs;
    return 0;
}

// SYS_write — write to file/stdout
// SysV: arg1(rdi)=fd, arg2(rsi)=buf, arg3(rdx)=count
uint64_t sys_write_handler(struct regs* regs)
{
    int fd = (int)regs->rdi;
    const void* buf = (const void*)regs->rsi;
    size_t count = (size_t)regs->rdx;

    if (fd == 1) {
        kwrite(buf, count);
        return count;
    }
    return (uint64_t)-1;
}

// SYS_read — read from file/stdin
// SysV: arg1(rdi)=fd, arg2(rsi)=buf, arg3(rdx)=count
uint64_t sys_read_handler(struct regs* regs)
{
    int fd = (int)regs->rdi;
    void* buf = (void*)regs->rsi;
    size_t count = (size_t)regs->rdx;

    if (fd == 0) {
        char* p = (char*)buf;
        size_t i = 0;
        while (i < count) {
            char c = serial_read_char();
            p[i++] = c;
            serial_write_char(c);
            if (c == '\n' || c == '\r') break;
        }
        return (uint64_t)i;
    }
    return (uint64_t)-1;
}

char open_path_table[16][256];

// SYS_open — open file
// SysV: arg1(rdi)=path, arg2(rsi)=flags
uint64_t sys_open_handler(struct regs* regs)
{
    const char* path = (const char*)regs->rdi;
    int flags = (int)regs->rsi;
    (void)flags;
    static int next_fd = 2;
    int fd = next_fd;
    if (next_fd < 15) next_fd++; else next_fd = 2;
    int i;
    for (i = 0; i < 255 && path[i]; i++)
        open_path_table[fd][i] = path[i];
    open_path_table[fd][i] = 0;
    return (uint64_t)fd;
}

// SYS_close — close file
uint64_t sys_close_handler(struct regs* regs)
{
    (void)regs;
    return 0;
}

// SYS_mmap — memory map
uint64_t sys_mmap_handler(struct regs* regs)
{
    (void)regs;
    return (uint64_t)-1;
}

// SYS_munmap — unmap memory
uint64_t sys_munmap_handler(struct regs* regs)
{
    (void)regs;
    return 0;
}

// SYS_sbrk — adjust heap
// arg1(rdi)=increment
uint64_t sys_sbrk_handler(struct regs* regs)
{
    intptr_t increment = (intptr_t)regs->rdi;
    uint64_t old = current_task->heap_end;
    uint64_t new = old + increment;
    if (current_task->heap_start == 0)
        current_task->heap_start = current_task->heap_end = USER_VIRT_START + 0x1000;
    current_task->heap_end = new;
    return old;
}

// SYS_sleep — sleep
// arg1(rdi)=seconds
uint64_t sys_sleep_handler(struct regs* regs)
{
    unsigned int seconds = (unsigned int)regs->rdi;
    uint64_t start = get_system_ticks();
    uint64_t end = start + seconds * 100;
    current_task->state = TASK_BLOCKED;
    while (get_system_ticks() < end) { /* busy wait */ }
    current_task->state = TASK_READY;
    return 0;
}

// SYS_execve — replace process image
// SysV: arg1(rdi)=path, arg2(rsi)=argv, arg3(rdx)=envp
uint64_t sys_execve_handler(struct regs* regs)
{
    const char* path = (const char*)regs->rdi;
    (void)regs;

    uint64_t entry_point;
    if (elf_load(path, &entry_point) < 0) {
        return (uint64_t)-1;
    }

    strncpy(current_task->name, path, sizeof(current_task->name) - 1);
    current_task->name[sizeof(current_task->name) - 1] = '\0';
    current_task->regs.rip = entry_point;

    serial_write_string("[SYSCALL] execve done\n");
    return 0;
}

// Initialize syscall: set up IA32_STAR / IA32_LSTAR MSRs for syscall/sysret
extern void syscall_stub(void);
void syscall_init(void)
{
    serial_write_string("[SYSCALL] init x86-64 syscall via MSR\n");

    // IA32_STAR: bits [47:32]=syscall CS/SS, bits [63:48]=sysret CS/SS
    //   kernel code at 0x08, kernel data at 0x10, user code at 0x1B, user data at 0x23
    uint64_t star = ((uint64_t)0x08 << 32) | ((uint64_t)0x1B << 48);
    __asm__ volatile("wrmsr" : : "a"(star & 0xFFFFFFFF), "d"(star >> 32), "c"(0xC0000081));

    // IA32_LSTAR: address of syscall entry (must be 64-bit)
    uint64_t lstar = (uint64_t)(uintptr_t)syscall_stub;
    __asm__ volatile("wrmsr" : : "a"(lstar & 0xFFFFFFFF), "d"(lstar >> 32), "c"(0xC0000082));

    // IA32_FMASK: RFLAGS mask (disable IF by default, can enable per-call)
    uint64_t fmask = 0x200;  // IF bit masked = 0 (interrupts enabled during syscalls)
    __asm__ volatile("wrmsr" : : "a"(fmask & 0xFFFFFFFF), "d"(fmask >> 32), "c"(0xC0000084));

    serial_write_string("[SYSCALL] MSRs written, syscall ready\n");
}
