#include <stdint.h>
#include <stddef.h>
#include "../include/drivers/io.h"

#define SERIAL_PORT 0x3F8  // COM1

static int is_initialized = 0;

void serial_init(void) {
    if (is_initialized) return;

    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x01);
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);

    is_initialized = 1;
}

static void serial_wait_ready(void) {
    while ((inb(SERIAL_PORT + 5) & 0x20) == 0);
}

void serial_putc(char c) {
    if (!is_initialized) serial_init();
    if (c == '\n') {
        serial_putc('\r');  // \n → \r\n
    }
    serial_wait_ready();
    outb(SERIAL_PORT, c);
}

void serial_puts(const char *str) {
    while (*str) {
        serial_putc(*str++);
    }
}

void serial_put_hex64(uint64_t v) {
    const char *hex = "0123456789ABCDEF";
    serial_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        serial_putc(hex[(v >> i) & 0xF]);
    }
}
