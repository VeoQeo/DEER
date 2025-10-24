#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <limine.h>

void fb_draw_pixel(struct limine_framebuffer *fb, uint32_t x, uint32_t y, uint32_t color);
void fb_fill_rect(struct limine_framebuffer *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_clear(struct limine_framebuffer *fb, uint32_t color);
#endif // FB_H