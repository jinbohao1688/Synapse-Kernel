ENTRY(_start)
SECTIONS {
    . = 0x100000;
    _kernel_start = .;
    .multiboot2 : { *(.multiboot2) }
    .text       : { *(.text) }
    .rodata     : { *(.rodata) }
    .data       : { *(.data) }
    .bss        : { *(COMMON) *(.bss) }
    _kernel_end = .;
}
