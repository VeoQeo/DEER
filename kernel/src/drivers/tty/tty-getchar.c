#include "../../include/drivers/tty/main-tty-struct.h"
#include "../../include/drivers/tty/tty-cursor.h"
#include "../../include/drivers/tty/tty-render.h"
#include "../../include/drivers/tty/tty-getchar.h"


// ЛОГИКА BACKSPACE '\b'
void backspace() {
    update_request_t request;               //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_BACKSPACE_INPUT;   // ~указываем мод обратового спейса

    terminal_update(&request);
}

// ЛОГИКА ENTER '\n'
void enter() {
    update_request_t request;            //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_ENTER_INPUT;    // ~указываем мод обратового спейса

    terminal_update(&request);
}

// ЛОГИКА ВОЗВРАТА КАРЕТКИ '\r'
void return_carriage() {
    update_request_t request;                   //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_RETURN_CARRIEGE_INPUT; // ~указываем мод возврат каретки

    terminal_update(&request);
}

// ЛОГИКА TAB '\t'
void tab() {
    update_request_t request;       //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_TAB_INPUT; // ~указываем мод табуляции

    terminal_update(&request);
}

// ЛОГИКА SPACE ' '
void space() {
    update_request_t request;         //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_SPACE_INPUT; // ~указываем мод пробела

    terminal_update(&request);
}

// ЗАПРАШИВАЕТ ВЫВОД 1 СИМВОЛА
void get_default_symbol(uint8_t symbol) {
    update_request_t request;                      //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_CHAR_INPUT;               // ~указываем мод символьного ввода
    request.params.char_input.symbol = symbol;   //<<передаем сам символ

    terminal_update(&request);
}

// ПРИНИМАЕТ 1 СИМВОЛ + ОБРАБОТКА
void terminal_getchar(uint8_t symbol) {
    switch (symbol) {
    case '\b':
        backspace();
        break;

    case '\n':
        enter();
        break;

    case '\r':
        return_carriage();
        break;

    case '\t':
        tab();
        break;

    case ' ':
        space();
        break;

    default:
        get_default_symbol(symbol);
        break;
    }
}
