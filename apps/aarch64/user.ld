OUTPUT_ARCH(aarch64)
ENTRY(_start)
PHDRS { code PT_LOAD FLAGS(5); ro PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); tls PT_TLS FLAGS(4); }
SECTIONS
{
    . = 0x40040000;
    .text : { KEEP(*(.text.start)) *(.text .text.*) } :code
    . = ALIGN(4096);
    .rodata : { *(.rodata .rodata.*) } :ro
    . = ALIGN(4096);
    .data : { *(.data .data.*) } :data
    .tdata : ALIGN(16) {
        __tls_start = .;
        *(.tdata .tdata.*)
        __tls_file_end = .;
    } :data :tls
    .tbss (NOLOAD) : { *(.tbss .tbss.*) __tls_end = .; } :data :tls
    __tls_size = __tls_end - __tls_start;
    __tls_align = MAX(16, MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)));
    __arm64_tls_tcb_offset = ALIGN(16, __tls_align);
    .bss (NOLOAD) : { *(.bss .bss.*) *(COMMON) } :data
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT(__tls_size <= 65536, "TLS image exceeds 64 KiB")
ASSERT(__tls_align <= 64, "TLS alignment exceeds 64 bytes")
