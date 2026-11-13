ENTRY(_start)

SECTIONS
{
    . = 0;
    .text : {
        *(.text.start)
        *(.text .text.*)
    }
    .rodata : { *(.rodata .rodata.*) }
    .eh_frame_hdr : {
        __eh_frame_hdr_start = .;
        *(.eh_frame_hdr)
        __eh_frame_hdr_end = .;
    }
    .eh_frame : {
        __eh_frame_start = .;
        KEEP(*(.eh_frame))
        __eh_frame_end = .;
    }
    .gcc_except_table : { *(.gcc_except_table .gcc_except_table.*) }
    . = ALIGN(0x2000);
    .init_array : {
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) .init_array))
        __init_array_end = .;
    }
    .fini_array : {
        __fini_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) .fini_array))
        __fini_array_end = .;
    }
    .data : { *(.data .data.*) *(.sdata .sdata.*) }
    .tdata : {
        __tdata_start = .;
        *(.tdata .tdata.*)
        __tdata_end = .;
    }
    .tbss : {
        __tbss_start = .;
        *(.tbss .tbss.*) *(.tcommon)
        __tbss_end = .;
    }
    __tls_size = __tbss_end - __tdata_start;
    .bss (NOLOAD) : {
        . = ALIGN(8);
        __bss_start = .;
        *(.bss .bss.* .sbss .sbss.*) *(COMMON)
        . = ALIGN(64);
        __tls_block = .;
        . += __tls_size;
        . = ALIGN(8);
        __bss_end = .;
    }
    /DISCARD/ : { *(.comment) *(.note.GNU-stack) *(.llvm_addrsig) }
}
