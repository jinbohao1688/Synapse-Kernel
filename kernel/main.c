#include <kernel.h>
#include <vga.h>
#include <string.h>
#include <multiboot2.h>
#include <common.h>
#include <keyboard.h>
#include <interrupts.h>
#include <shell.h>
#include <mm/paging.h>
#include <proc.h>
#include <fs.h>

void kernel_main(uint32_t magic, uint32_t mbi_addr)
{
    UNUSED(magic);
    UNUSED(mbi_addr);

    vga_init();

    kprint("Synapse OS v");
    kprint(KERNEL_VERSION);
    kprint("\n");
    kprint("AI-Native Operating System\n");
    kprint("Initializing...\n");

    kprint("VGA driver initialized\n");

    keyboard_init();
    kprint("Keyboard driver initialized\n");

    idt_init();
    kprint("IDT initialized\n");

    init_paging();
    kprint("Paging initialized\n");

    init_kheap();
    kprint("Kernel heap initialized\n");

    tasking_init();
    kprint("Process scheduling initialized\n");

    syscall_init();
    kprint("System call initialized\n");

    init_filesystem();
    kprint("Filesystem initialized\n");

    init_procfs();
    kprint("Proc filesystem initialized\n");

    irq_install();
    kprint("IRQ installed\n");

    kprint("System ready\n");

    shell_init();
    shell_run();
}
