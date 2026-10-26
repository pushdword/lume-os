OUTPUT_FORMAT(elf32-x32)
ENTRY(_start)
/* Default keeps the tested64MiB layout. A validated pager profile can supply
 * --defsym=__lume_user_limit=0x48000000 without weakening the normal guard. */
PROVIDE(__lume_user_limit = 0x44000000);
PHDRS { code PT_LOAD FLAGS(5); ro PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
 . = 0x40000000;
 .text : { KEEP(*(.text.entry)) *(.text .text.*) } :code
 .rodata ALIGN(4096) : { *(.rodata .rodata.*) } :ro
 .data ALIGN(4096) : { *(.data .data.*) } :data
 .init_array ALIGN(4) : {
   __init_array_start = .;
   KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*))) KEEP(*(.init_array))
   __init_array_end = .;
 } :data
 .fini_array ALIGN(4) : {
   __fini_array_start = .;
   KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*))) KEEP(*(.fini_array))
   KEEP(*(.fini_array_onexit))
   __fini_array_end = .;
 } :data
 .bss ALIGN(8) (NOLOAD) : { *(.bss .bss.* COMMON) } :data
 __heap_start = ALIGN(4096);
 /* sbrk obtains dynamic pages through BRK35. This symbol supports older libc units. */
 __heap_end = __heap_start;
 ASSERT(__heap_start < (__lume_user_limit - 0x102000), "Native image leaves no room for heap/stack")
 /DISCARD/ : { *(.comment .eh_frame .note.*) }
}
