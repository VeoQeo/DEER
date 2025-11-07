#include <stdbool.h>
#include <stdint.h>

#include "../../include/drivers/tty/main-tty-struct.h"
#include "../../include/drivers/tty/tty-render.h"


// ПРОКРУТИТЬ ВНИЗ
void scroll_down(int count) {
    update_request_t request;      //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ПРОКРУТКУ ВНИЗ
    request.mode = MODE_SCROLL_DOWN;
    request.params.scroll_down.lines = count;

    terminal_update(&request); // отправляем запрос на отрисовку
}

// ПРОКРУТИТЬ ВВЕРХ
void scroll_up(int count) {
    update_request_t request; //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ПРОКРУТКУ ВВЕРХ
    request.mode = MODE_SCROLL_DOWN;
    request.params.scroll_down.lines = count;

    terminal_update(&request); // отправляем запрос на отрисовку
}

// УСТАНОВИТЬ ЦВЕТ
void terminal_set_color(uint8_t symbol_color_theme, uint8_t background_color_theme) {
    update_request_t request; //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ИЗМЕНЕНИЕ ТЕМЫ
    request.mode = MODE_SET_COLOR_THEME;

    request.params.set_color.symbol_color_theme     = symbol_color_theme;
    request.params.set_color.background_color_theme = background_color_theme;

    terminal_update(&request); // отправляем запрос на отрисовку
}

// ПОЛУЧИТЬ ИНФОРМАЦИЮ ОБ УСТАНОВЛЕННОМ ЦВЕТЕ (ПРЯМОЙ ЗАПРОС В TTY)
uint8_t terminal_get_color(layer_t layer) {
    struct tty_parameters *parameter_info = interact_tty_parameters(); //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ СТРУКТУРЫ TTY

    if (layer == BACKGROUND) {
        return parameter_info -> tty_background_color_theme;

    } else if (layer == SYMBOL) {
        return parameter_info -> tty_symbol_color_theme;
    }

    return 0;
}
