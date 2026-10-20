ENTRY(_start)
PHDRS { code PT_LOAD FLAGS(5); ro PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
 . = 0x40000000;
 .text : { KEEP(*(.text.entry)) *(.text .text.*) } :code
 .rodata ALIGN(4096) : { *(.rodata .rodata.*) } :ro
 ASSERT(. <= 0x40010000, "User code exceeds64KiB window")
 . = 0x40010000;
 .data : { *(.data .data.*) } :data
 .bss ALIGN(8) (NOLOAD) : {
     *(.bss .bss.* COMMON) . = ALIGN(8); __heap_start = .;
     ASSERT(. <= 0x40018000, "User globals exceed data window")
     . = 0x40018000; __heap_end = .;
 } :data
 ASSERT(. <= 0x40018000, "User data exceeds32KiB window")
 /DISCARD/ : { *(.comment .eh_frame .note.*) }
}
