// kernel/include/graphics/color.h
#ifndef COLOR_H
#define COLOR_H

#include <stdint.h>

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))

#define COLOR_BLACK     RGB(  0,   0,   0)
#define COLOR_WHITE     RGB(255, 255, 255)
#define COLOR_RED       RGB(255,   0,   0)
#define COLOR_GREEN     RGB(  0, 255,   0)
#define COLOR_BLUE      RGB(  0,   0, 255)
#define COLOR_CYAN      RGB(  0, 255, 255)
#define COLOR_MAGENTA   RGB(255,   0, 255)
#define COLOR_YELLOW    RGB(255, 255,   0)
#define COLOR_ORANGE    RGB(255, 165,   0)
#define COLOR_GRAY      RGB(128, 128, 128)
#define COLOR_DARKGRAY  RGB( 64,  64,  64)
#define COLOR_BROWN     RGB(165,  42,  42)

#endif // COLOR_H