ENTRY(_start)

SECTIONS {
    . = 0x0000000000400000;   /* 4MB, standard user address */

    .text : {
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        *(.bss)
        *(.bss.*)
    }

    __stack = .;
    __stack_end = . + 4096;
}
