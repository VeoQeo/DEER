#ifndef BANNER_H
#define BANNER_H

#include <stdint.h>
#include <limine.h>

void draw_deer_banner(struct limine_framebuffer *fb, uint32_t x, uint32_t y, uint32_t color);

#endif // BANNER_H