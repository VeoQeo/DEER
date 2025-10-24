#include <stdint.h>
#include <stddef.h>
#include "../include/drivers/io.h"

#define SERIAL_PORT 0x3F8  // COM1

static int is_initialized = 0;

void serial_init(void) {
    if (is_initialized) return;

    // Отключаем прерывания
    outb(SERIAL_PORT + 1, 0x00);
    // Устанавливаем скорость: Divisor Latch Access Bit (DLAB)
    outb(SERIAL_PORT + 3, 0x80);
    // Делитель скорости: 115200 / 115200 = 1 (скорость: 115200)
    outb(SERIAL_PORT + 0, 0x01);
    outb(SERIAL_PORT + 1, 0x00);
    // 8 бит данных, нет чётности, 1 стоп-бит
    outb(SERIAL_PORT + 3, 0x03);
    // Включаем FIFO, очищаем буферы
    outb(SERIAL_PORT + 2, 0xC7);
    // IRQs enabled, RTS/DSR set
    outb(SERIAL_PORT + 4, 0x0B);

    is_initialized = 1;
}

// Ждём, пока порт будет готов к передаче
static void serial_wait_ready(void) {
    while ((inb(SERIAL_PORT + 5) & 0x20) == 0);
}

// Отправить один символ
void serial_putc(char c) {
    if (!is_initialized) serial_init();
    if (c == '\n') {
        serial_putc('\r');  // \n → \r\n
    }
    serial_wait_ready();
    outb(SERIAL_PORT, c);
}

// Отправить строку
void serial_puts(const char *str) {
    while (*str) {
        serial_putc(*str++);
    }
}