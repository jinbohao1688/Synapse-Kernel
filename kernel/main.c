#include <kernel.h>
#include <vga.h>
#include <string.h>
#include <multiboot2.h>
#include <common.h>
#include <keyboard.h>
#include <interrupts.h>
#include <shell.h>
#include <mm/paging.h>
#include <mm/kheap.h>
#include <proc.h>
#include <fs.h>
#include <proc/task.h>
#include <serial.h>

extern void init_procfs(void);

void kernel_main(uint32_t magic, uint32_t mbi_addr)
{
    UNUSED(magic);
    UNUSED(mbi_addr);

    /* ── 最早期初始化 ────────────────────────────────── */
    serial_init();
    serial_write_string("kernel_main entered\n");

    vga_init();
    serial_write_string("[OK] vga_init\n");
    kprint("Synapse OS v"); kprint(KERNEL_VERSION); kprint("\n");

    keyboard_init();
    serial_write_string("[OK] keyboard_init\n");

    __asm__ volatile("cli");
    idt_init();
    serial_write_string("[OK] idt_init\n");

    /* ── 内存初始化 ──────────────────────────────────── */
    init_paging();
    serial_write_string("[OK] init_paging\n");

    init_kheap();
    serial_write_string("[OK] init_kheap\n");

    /* ── 堆验证（确认后可删） ────────────────────────── */
    volatile uint32_t *probe = (volatile uint32_t *)KHEAP_START;
    *probe = 0xDEADBEEFu;
    if (*probe == 0xDEADBEEFu)
        serial_write_string("[VERIFY] heap rw: PASS\n");
    else
        serial_write_string("[VERIFY] heap rw: FAIL\n");

    void *p = kmalloc(64);
    if (p) serial_write_string("[VERIFY] kmalloc: PASS\n");
    else   serial_write_string("[VERIFY] kmalloc: FAIL\n");
    /* ────────────────────────────────────────────────── */

    serial_write_string("[OK] Memory OK\n");

    /* ── 子系统初始化 ────────────────────────────────── */
    serial_write_string("[..] sched_init\n");
    sched_init();
    serial_write_string("[OK] sched_init\n");

    serial_write_string("[..] syscall_init\n");
    syscall_init();
    serial_write_string("[OK] syscall_init\n");

    serial_write_string("[..] init_filesystem\n");
    init_filesystem();
    serial_write_string("[OK] init_filesystem\n");

    serial_write_string("[..] init_procfs\n");
    init_procfs();
    serial_write_string("[OK] init_procfs\n");

    serial_write_string("[..] irq_install\n");
    irq_install();
    serial_write_string("[OK] irq_install\n");
    /* 开启中断，serial_read_char 用 hlt 等待 */
    /* 不开中断，用忙等读串口 */

    /* ── 进入 shell ──────────────────────────────────── */
    serial_write_string("[OK] all init done, entering shell\n");
    kprint("System ready\n");
    shell_init();
    shell_run();
}