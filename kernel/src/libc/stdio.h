#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

void printf_set_framebuffer(struct limine_framebuffer *fb);
void printf_set_cursor(uint32_t x, uint32_t y);
void printf_set_color(uint32_t color);
void printf_set_bg_color(uint32_t color);
void printf_get_cursor(uint32_t *x, uint32_t *y);
void printf_clear(void);

int printf(const char *format, ...);
int puts(const char *str);
void printf_init_with_framebuffer(struct limine_framebuffer *fb);
void printf_uint64(uint64_t value);

#endif // STDIO_H