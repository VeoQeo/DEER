ENTRY(_start)

SECTIONS {
    . = 0x100000;

    .boot BLOCK(4K) : ALIGN(4K) {
        KEEP(*(.multiboot_header))
    }

    .text : ALIGN(4K) {
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data)
    }

    .bss : ALIGN(4K) {
        *(.bss)
        *(COMMON)
    } = 0
}