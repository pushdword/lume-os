OUTPUT_ARCH(aarch64)
ENTRY(_start)
PHDRS { code PT_LOAD FLAGS(5); ro PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS
{
    . = 0x40040000;
    .text : { KEEP(*(.text.start)) *(.text .text.*) } :code
    . = ALIGN(4096);
    .rodata : { *(.rodata .rodata.*) } :ro
    . = ALIGN(4096);
    .data : { *(.data .data.*) } :data
    .init_array : {
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*))) KEEP(*(.init_array))
        __init_array_end = .;
    } :data
    .fini_array : {
        __fini_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*))) KEEP(*(.fini_array))
        __fini_array_end = .;
    } :data
    .bss (NOLOAD) : { *(.bss .bss.*) *(COMMON) } :data
    ASSERT(. <= 0x48000000, "Native image exceeds the executable region")
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
