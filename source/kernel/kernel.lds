SECTIONS {
    PROVIDE(kernbase = .);
    . = 0x10000;

    PROVIDE(text_start_addr = .);
    .text : {
        *(.text) 
    }

    .rodata : {
        *(.rodata)
    }
    PROVIDE(text_end_addr = .);

    . = ALIGN(4096);
    PROVIDE(data_start_addr = .);
    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    PROVIDE(mem_free_start = .);
}