ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    _kernel_start = .;

    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;

    /* Page directory, page tables, and bitmap go AFTER kernel BSS.
     * We use fixed addresses starting at 0x110000 so they never
     * overlap with kernel code/data (which ends ~0x10E000).
     *
     * Why 0x110000? Because the temporary PD/PT in the old code
     * were also at 0x105000-0x10FFFF. By moving to 0x110000 we
     * keep them in the lower 8MB identity-mapped region and avoid
     * any overlap with kernel BSS.
     *
     * NOTE: kernel_pd MUST be identity-mapped by PD[0]/PT0 so we
     * can write to it directly from kernel code (paging OFF initially).
     * 0x110000 is in the 0-4MB range (mapped by PT0) ✓ */
    . = 0x110000;
    _page_dir_addr = .;        /* 1 page for page directory     */
    . = 0x111000;
    _kernel_pt0_addr = .;     /* 1 page for kernel PT0        */
    . = 0x112000;
    _kernel_pt1_addr = .;     /* 1 page for kernel PT1        */
    . = 0x113000;
    _frame_bitmap_addr = .;   /* bitmap (1+ frames)            */

    . = ALIGN(4096);
    _bss_resume = .;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
