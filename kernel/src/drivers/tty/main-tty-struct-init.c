#include <stdbool.h>
#include "../../libc/string.h"

#include "../../include/drivers/tty/main-tty-struct.h"
#include "../../include/drivers/tty/tty-render.h"


// ~ИНИЦИАЛИЗИРУЕМ СТРУКТУРУ
static struct tty_parameters parameter_info = {
    .service_field = {
        .blended_colors   = {{0}},
        .symbols          = {{0}},
        .user_name        = NULL,
        .user_name_length = 0
    },
    .backlog = {
        .blended_colors = {{0}},
        .symbols = {{0}}
    },
    .cursor_position = {
        .cursor_shape = {
            .pixel_start_line = CURSOR_SHAPE_UNDERLINE_START_VALUE,
            .pixel_end_line   = CURSOR_SHAPE_UNDERLINE_END_VALUE,

            .isBlink_enable = true,
            .isVisible      = true
        },
        .io_index_port = IO_INDEX_PORT_COLOR,
        .io_data_port  = IO_DATA_PORT_COLOR,

        .y = 0,  
        .x = VGA_SERVICE_LENGTH   
    },

    .tty_symbol_color_theme     = VGA_COLOR_GREEN,     
    .tty_background_color_theme = VGA_COLOR_BLACK, 

    .vga_memory_address = VGA_MEMORY_ADDRESS_COLOR,
    .isEnter = false,

    .getchar_buffer        = {0},
    .getchar_buffer_length = 0,
    .getchar_start_at      = 0,

    .putchar_buffer    = {{0}},
    .putchar_free_cell = {0, 0},
    
    .upper_limit = 0,
    .lower_limit = VGA_OUTPUT_HEIGHT
};

// ЗАПОЛНИТЬ ВСЮ СЕТКУ ПРОБЕЛАМИ
void terminal_init() {
    update_request_t request;                  //<<СОЗДАЕМ ОБРАЩЕНИЕ НА ЗАПОЛНЕНИЕ ВИДЕОБУФЕРА
    request.mode = MODE_INIT;                 //<<РЕЖИМ ИНИЦИАЛИЗАЦИИ
    
    char *user_name = USER_NAME;
    // request.params.init_data.user_name = user_name;                 //<<заглушка
    // request.params.init_data.user_name_length = strlen(user_name); //<<заглушка

    uint8_t tty_background_color_theme = VGA_COLOR_BLACK;
    uint8_t tty_symbol_color_theme     = VGA_COLOR_GREEN;

    request.params.init_data.tty_background_color_theme = tty_background_color_theme;
    request.params.init_data.tty_symbol_color_theme     = tty_symbol_color_theme;

/*
    // 1. Инициализируем данные сетки стартовыми данными 
    for (int i = 0; i < VGA_OUTPUT_HEIGHT; i++) {
        for (int j = 0; j < VGA_OUTPUT_LENGTH; j++) {
            request.params.init_data.blended_color_buffer[i][j] = vga_blend_color(tty_background_color_theme, init_color_symbol); // мешаем цвет фона и символа
            request.params.init_data.symbol_buffer[i][j] = ' ';
        }
    }
*/

    request.params.init_data.y = 0;                                   //<<если отрисуем лого, то сдвигаем на кол-во занятых строк, по умолчанию 0.
    request.params.init_data.x = VGA_SERVICE_LENGTH;   
  
    request.params.init_data.isEnter = true;                       //<<просим вывести имя пользователя
  
    request.params.init_data.upper_limit = 0;                    //<<ВЕРХНЯЯ ГРАНИЦА ВИДИМОГО ЭКРАНА
    request.params.init_data.lower_limit = VGA_OUTPUT_HEIGHT;   //<<НИЖНЯЯ ГРАНИЦА ВИДИМОГО ЭКРАНА

    terminal_update(&request); // отправляем запрос на отрисовку

}

// ПЕРЕДАЕМ УКАЗАТЕЛЬ НА СТРУКТУРУ ТОЛЬКО В ТЕ МЕСТА, ГДЕ БУДЕМ ОБРАЩАТЬСЯ К ЭТОЙ СТРУКТУРЕ
struct tty_parameters *interact_tty_parameters(void) {
    return &parameter_info;
}
