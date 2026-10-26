OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
    . = 0x600000;
    __image_start = .;
    .text : { KEEP(*(.text.boot)) *(.text .text.*) }
    . = ALIGN(4096);
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    __load_end = .;
    . = ALIGN(16);
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.*) *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    .stack (NOLOAD) : {
        . = ALIGN(4096);
        . += 0x10000;
        __stack_top = .;
    }
    __image_end = .;
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT((_start == 0x600000), "Recovery image entry must be 0x600000")
ASSERT((__image_end <= 0x800000), "Recovery image and stack exceed the reserved 2 MiB")
ASSERT(((__bss_end - __bss_start) % 16 == 0), "Recovery BSS alignment")
