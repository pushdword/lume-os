OUTPUT_ARCH(aarch64)
ENTRY(_start)
PHDRS { code PT_LOAD FLAGS(5); ro PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); tls PT_TLS FLAGS(4); }
SECTIONS
{
    . = 0x4000000000;
    .text : { __lume_text_start = .; KEEP(*(.text.start)) *(.text .text.*) KEEP(*(malloc_hook malloc_hook.*)) __lume_text_end = .; } :code
    . = ALIGN(4096);
    .rodata : { *(.rodata .rodata.*) } :ro
    .note.gnu.build-id : {
        __lume_build_id_start = .;
        KEEP(*(.note.gnu.build-id))
        __lume_build_id_end = .;
    } :ro
    . = ALIGN(4096);
    .data : { *(.data .data.*) } :data
    .preinit_array : {
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
    } :data
    .init_array : {
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*))) KEEP(*(.init_array))
        __init_array_end = .;
    } :data
    .fini_array_onexit : {
        __fini_array_onexit_start = .;
        KEEP(*(.fini_array_onexit))
        __fini_array_onexit_end = .;
    } :data
    .fini_array : {
        __fini_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*))) KEEP(*(.fini_array))
        __fini_array_end = .;
    } :data
    .got : { *(.got .got.*) } :data
    .tdata : ALIGN(MAX(16, ALIGNOF(.tbss))) {
        __tls_start = .;
        *(.tdata .tdata.*)
        __tls_file_end = .;
    } :data :tls
    .tbss (NOLOAD) : { *(.tbss .tbss.*) __tls_end = .; } :data :tls
    __tls_size = __tls_end - __tls_start;
    __tls_align = MAX(16, MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)));
    __arm64_tls_tcb_offset = ALIGN(16, __tls_align);
    .bss (NOLOAD) : { *(.bss .bss.*) *(COMMON) } :data
    ASSERT(. <= 0x4100000000, "Native image exceeds the executable region")
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT(__tls_size <= 65536, "TLS image exceeds 64 KiB")
ASSERT(__tls_align <= 64, "TLS alignment exceeds 64 bytes")
