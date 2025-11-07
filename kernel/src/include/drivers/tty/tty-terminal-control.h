#ifndef TERMINAL_CONTROL_H
#define TERMINAL_CONTROL_H 

#include <stdbool.h>
#include <stdint.h>

void scroll_down(int count);
void scroll_up(int count);
void terminal_set_color(uint8_t symbol_color_theme, uint8_t background_color_theme);
uint8_t terminal_get_color(layer_t layer);

#endif