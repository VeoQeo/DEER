#include <stdint.h>

#include "../../include/drivers/tty/main-tty-struct.h"
#include "../../include/drivers/tty/tty-cursor.h"
#include "../../include/drivers/tty/tty-render.h"
#include "../../include/drivers/tty/tty-putchar.h"

/*tty-esc-sequences*/

// ЛОГИКА БЕКСПЕЙСА (\b)
void back_slash_b() {
    update_request_t request;               //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_BACKSPACE_OUTPUT;  // ~указываем мод возврат каретки 

    terminal_update(&request);
}

// ЛОГИКА ПЕРЕНОСА СТРОКИ (\n)
void back_slash_n() {
    update_request_t request;             //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_ENTER_OUTPUT;    // ~указываем мод возврат каретки 

    terminal_update(&request);
}

// ЛОГИКА ВОЗВРАТА КАРЕТКИ (\r)
void back_slash_r() {
    update_request_t request;                       //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_RETURN_CARRIAGE_OUTPUT;    // ~указываем мод возврат каретки 

    terminal_update(&request);
}

/*
    // ЛОГИКА СИГНАЛА (\a)
    void back_slash_a(struct tty_parameters *parameter_info) {
        update_request_t request;           //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
        request.mode = MODE_TAB_OUTPUT;    // ~указываем мод обратового спейса

        terminal_update(&request);
    }
*/

// ЛОГИКА ТАБУЛЯЦИИ (\t)
void back_slash_t() {
    update_request_t request;           //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_TAB_OUTPUT;    // ~указываем мод таба

    terminal_update(&request);
}

// ЛОГИКА ВЫВОДА ДЕФОЛТНОГО СИМВОЛА
void put_default_symbol(char symbol) { 
    update_request_t request;            //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_CHAR_OUTPUT;    // ~указываем мод таба
    request.params.char_output.symbol = symbol;

    terminal_update(&request);
}

void terminal_putchar(char symbol) {

    switch (symbol) {
    case '\b':
        back_slash_b();
        break;

    case '\n':
        back_slash_n();
        break;

    case '\r':
        back_slash_r();
        break;

    case '\t':
        back_slash_t();
        break;

    default:
        put_default_symbol(symbol);
        break;
    }
}