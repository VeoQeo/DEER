#ifndef TTY_CELL_H
#define TTY_CELL_H

#include "main-tty-struct.h"

uint8_t vga_blend_color(vga_colors_t fg, vga_colors_t bg);
uint16_t vga_cell(unsigned char uc, uint8_t color);

#endif