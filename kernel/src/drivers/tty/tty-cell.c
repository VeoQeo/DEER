#include <stdint.h>

#include "../../include/drivers/tty/main-tty-struct.h"
#include "../../include/drivers/tty/tty-cell.h"


// ПРИНИМАЕТ ДВА 4-Х БИТНЫХ ЧИСЛА, ВОЗВРАЩАЕТ 8 БИТНОЕ ЧИСЛО (ПЕРВЫЕ 4 БИТА ФОН, ОСТАЛЬНОЕ - ЦВЕТ ВОЗВРАЩАЕМОГО СИМВОЛА)
uint8_t vga_blend_color(vga_colors_t fg, vga_colors_t bg) {
    return fg | bg << 4;
}

// ПРИНИМАЕТ СИМВОЛ (ASCII) И ПОЛУЧЕННЫЙ ОТ vga_blend_color 8-МИ БИТНЫЙ ЦВЕТ, ВОЗВРАЩАЕТ СИМВОЛ И ЦВЕТ
uint16_t vga_cell(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

