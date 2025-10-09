ENTRY(_start)

SECTIONS {
  . = 1M;
  .boot BLOCK(4K) : ALIGN(4K) {
    KEEP(*(.multiboot))
  }
  .text BLOCK(4K) : ALIGN(4K) {
    *(.text)
  }
  .data BLOCK(4K) : ALIGN(4K) {
    *(.data)
  }
  .bss BLOCK(4K) : ALIGN(4K) {
    *(COMMON)
    *(.bss)
  }
}