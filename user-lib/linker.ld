ENTRY(_start)

SECTIONS {
    . = 0x08048000;  /* ELF standard entry point address */
    
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
    __stack_end = . + 4096;  /* 4KB stack */
}
