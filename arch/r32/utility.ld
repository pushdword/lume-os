ENTRY(_start)
PHDRS { code PT_LOAD FLAGS(5); ro PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
 . = 0x40000000;
 .text : { KEEP(*(.text.entry)) *(.text .text.*) } :code
 .rodata ALIGN(4096) : { *(.rodata .rodata.*) } :ro
 ASSERT(. <= 0x40100000, "Utility code exceeds 1 MiB")
 . = ALIGN(4096);
 .data : { *(.data .data.*) } :data
 .bss ALIGN(8) (NOLOAD) : {
     *(.bss .bss.* COMMON) . = ALIGN(8); __heap_start = .;

     . += 0x20000; __heap_end = .;
 } :data

 /DISCARD/ : { *(.comment .eh_frame .note.*) }
}
