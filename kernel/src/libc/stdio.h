#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

// Функции управления выводом
void printf_set_framebuffer(struct limine_framebuffer *fb);
void printf_set_cursor(uint32_t x, uint32_t y);
void printf_set_color(uint32_t color);
void printf_set_bg_color(uint32_t color);
void printf_get_cursor(uint32_t *x, uint32_t *y);
void printf_clear(void);

// Основные функции вывода
int printf(const char *format, ...);
int puts(const char *str);

#endif // STDIO_H