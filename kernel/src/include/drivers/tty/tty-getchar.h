#ifndef TTY_PUTCHAR_H
#define TTY_PUTCHAR_H

#include <stdint.h>

void backspace();
void enter();
void return_carriage();
void tab();
void space();
void get_default_symbol(uint8_t symbol);
void terminal_getchar(uint8_t symbol);

#endif