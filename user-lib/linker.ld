ENTRY(_start)

SECTIONS {
    . = 0x00400000;  /* 4MB, within identity-mapped 0-8MB */
    
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
