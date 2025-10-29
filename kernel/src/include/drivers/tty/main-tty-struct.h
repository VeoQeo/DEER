#ifndef MAIN_TTY_STRUCT_H
#define MAIN_TTY_STRUCT_H

#include <stdbool.h>
#include <stdint.h>

// ВИДИМАЯ ЧАСТЬ
#define VGA_OUTPUT_HEIGHT 25     // [ЗНАЧЕНИЕ]: максимальное количество строк в видимом экране
#define VGA_OUTPUT_LENGTH 80    //  [ЗНАЧЕНИЕ]: максимальная длина строки

// РАЗМЕРЫ СТРУКТУРЫ service_field
#define VGA_SERVICE_HEIGHT 75   // [ЗНАЧЕНИЕ]: высота истории служебных данных
#define VGA_SERVICE_LENGTH 25  //  [ЗНАЧЕНИЕ]: ширина границ служебных данных

// РАЗМЕРЫ СТРУКТУРЫ backlog
#define VGA_BACKLOG_HEIGHT 75                                       // [ЗНАЧЕНИЕ]: высота истории пользовательских 
#define VGA_BACKLOG_LENGTH VGA_OUTPUT_LENGTH - VGA_SERVICE_LENGTH  //  [ЗНАЧЕНИЕ]: длина беклога (в данной версии используем VGA_OUTPUT_LENGTH)

// ПАРАМЕТРЫ БУФЕРА ДЛЯ getchar
#define GETCHAR_MAX_LENGTH 80    // [ЗНАЧЕНИЕ]: 80 символов в ряд

// ПАРАМЕТРЫ СЕТКИ ДЛЯ putchar 
#define PUTCHAR_MAX_LENGTH VGA_OUTPUT_LENGTH                      // [ЗНАЧЕНИЕ]: длина сетки для вывода символа 
#define PUTCHAR_MAX_HEIGHT 75                                    //  [ЗНАЧЕНИЕ]: высота сетки для вывода символа 

// РЕГИСТРЫ ДЛЯ УПРАВЛЕНИЯ КУРСОРОМ
#define CRTC_REGISTER_CURSOR_START_LINE 0x0A           // [НОМЕР РЕГИСТРА]: начальная пиксельная строка курсора (и бит мигания)
#define CRTC_REGISTER_CURSOR_END_LINE 0x0B            //  [НОМЕР РЕГИСТРА]: конечная пиксельная строка курсора
#define CRTC_REGISTER_CURSOR_HIGH_POSITION 0x0E      //   [НОМЕР РЕГИСТРА]: старший байт позиции курсора
#define CRTC_REGISTER_CURSOR_LOW_POSITION 0x0F      //    [НОМЕР РЕГИСТРА]: младший байт позиции курсора

// ЗНАЧЕНИЯ ДЛЯ ФОРМЫ КУРСОРА
#define CURSOR_SHAPE_UNDERLINE_START_VALUE 0x0E    // [ЗНАЧЕНИЕ]: для CRTC_REGISTER_CURSOR_START_LINE
#define CURSOR_SHAPE_UNDERLINE_END_VALUE   0x0F   //  [ЗНАЧЕНИЕ]: для CRTC_REGISTER_CURSOR_END_LINE

// БИТЫ ДЛЯ УПРАВЛЕНИЯ ПОВЕДЕНИЕМ КУРСОРА
#define CURSOR_BLINK_ENABLE_BIT 0x20         // [БИТ 5]: включить мигание курсора
#define CURSOR_DISABLE_BIT 0x40             //  [БИТ 6]: полностью выключить курсор

// АДРЕСА/АДРЕСА ПОРТОВ ДЛЯ ЦВЕТНОГО VGA
#define VGA_MEMORY_ADDRESS_COLOR ((uint16_t *)0xB8000)   // [АДРЕС ВИДЕОПАМЯТИ]: для цветного режима
#define IO_INDEX_PORT_COLOR 0x3D4                       //  [АДРЕС ПОРТА]: для выбора регистра CRTC (цветной)
#define IO_DATA_PORT_COLOR 0x3D5                       //   [АДРЕС ПОРТА]: для данных регистра CRTC (цветной)

// ПАРАМЕТРЫ ESC-SEQUENCES
#define TAB   4  // размер tab
#define SPACE 1 

// [ЗАГЛУШКА]: ИМЯ ПОЛЬЗОВАТЕЛЯ, ПОКА СТАТИЧЕСКИЙ ДЛЯ ТЕСТОВ
#define USER_NAME "neverbalny_pahich: "
#define USER_NAME_LENGTH 8 


// ОП-КОДЫ ЦВЕТОВ
typedef enum {
    VGA_COLOR_BLACK = 0x00,
    VGA_COLOR_BLUE = 0x01,
    VGA_COLOR_GREEN = 0x02,
    VGA_COLOR_CYAN = 0x03,
    VGA_COLOR_RED = 0x04,
    VGA_COLOR_MAGENTA = 0x05,
    VGA_COLOR_BROWN = 0x06,
    VGA_COLOR_LIGHT_GREY = 0x07,
    VGA_COLOR_DARK_GREY = 0x08,
    VGA_COLOR_LIGHT_BLUE = 0x09,
    VGA_COLOR_LIGHT_GREEN = 0xA,
    VGA_COLOR_LIGHT_CYAN = 0xB,
    VGA_COLOR_LIGHT_RED = 0xC,
    VGA_COLOR_LIGHT_MAGENTA = 0xD,
    VGA_COLOR_LIGHT_BROWN = 0xE,
    VGA_COLOR_WHITE = 0xF,

} vga_colors_t;

// СЛОИ ДЛЯ Ф-Й ПОЛУЧЕНИЯ/УСТАНОВКИ ЦВЕТА
typedef enum {
    SYMBOL,
    BACKGROUND

} layer_t;


// СТРУКТУРА TTY
struct tty_parameters {
/*храним имя пользователя и границы левой сетки*/
    struct prompt_field_storage {
        uint8_t blended_colors[VGA_SERVICE_HEIGHT][VGA_SERVICE_LENGTH];    // тут цвет символа и ячейки (пока цвет для пользовательских сомволов одинаковый для служебных)
        uint8_t        symbols[VGA_SERVICE_HEIGHT][VGA_SERVICE_LENGTH];   //  тут храним имя пользователя и тек. директорию. Если директория вылазит за рамки, то сокращаем название пути

        char *user_name;     // храним имя пользователя, пока заглушка
        int user_name_length;  //  храним длину пользователя + пробел --> user:_, пока заглушка

    } service_field;
    
/*храним историю о всех введенных символах правой сетки (включая цвета) до 75 строки*/
    struct vga_backlog_storage {
        uint8_t blended_colors[VGA_BACKLOG_HEIGHT][VGA_OUTPUT_LENGTH];
        uint8_t        symbols[VGA_BACKLOG_HEIGHT][VGA_OUTPUT_LENGTH];

    } backlog;

    /*координаты курсора и его параметры*/
    struct pointer {
        struct pointer_data {
            uint8_t pixel_start_line;   // начальная пиксельная строка курсора (например, 0x0E для подчеркивания)
            uint8_t pixel_end_line;    //  конечная пиксельная строка курсора (например, 0x0F для подчеркивания)

            bool isBlink_enable;   // флаг: 1, если мигание включено; 0, если выключено
            bool isVisible;       //  флаг: 1, если курсор должен быть виден; 0, если скрыт (например, при потере фокуса)

        } cursor_shape;

        uint16_t io_index_port;
        uint16_t io_data_port;

        int y; // ВАЖНО! ИНДЕКС y РАВЕН service_field.symbols[<этой координате>][...], то есть может быть фактически больше 25! НО, передавая в буфер, мы не можем превышать 25, поэтому урезаем его в ф-ии render() по upper_limit  
        int x;  

    } cursor_position;

/*инициализируемые параметры*/
    uint8_t tty_symbol_color_theme;
    uint8_t tty_background_color_theme;

    uint16_t *vga_memory_address;

/*временные параметры*/
    bool isEnter; //<<если true, значит пользователь ввел enter. Переходим на новую строку, очищаем сommand_buffer и сommand_length, распечатываем имя пользователя, добавляем к верхней и нижней границам 1

    /*stdin*/
    uint8_t getchar_buffer[GETCHAR_MAX_LENGTH];      // сюда помещаем введенные символы с клавиатуры ДО нажатия на enter БЕЗ ЦВЕТОВ! render() при отрисовки возьмет цвета из tty_symbol_color_theme и tty_background_color_theme
    int getchar_buffer_length;                      //  храним кол-во введенных символов
    int getchar_start_at;                          //   ВАЖНО! ИНДЕКС getchar_start_at РАВЕН service_field.symbols[<этой координате>][...], то есть может быть фактически больше 25!

    /*stdout*/
    uint8_t putchar_buffer[PUTCHAR_MAX_HEIGHT][PUTCHAR_MAX_LENGTH];  // ВАЖНО! PUTCHAR_MAX_LENGTH = VGA_OUTPUT_LENGTH 
    int putchar_free_cell[2];                                       //  храним координату СВОБОДНОЙ ЯЧЕЙКИ (начало всегда с x = VGA_SERVICE_LENGTH)

    /*текущие отрисованные границы беклога*/
    int upper_limit;   // верхняя граница отображаемого экрана
    int lower_limit;  //  нижняя граница отображаемого экрана

};


struct tty_parameters *interact_tty_parameters(void);  // передача указателя на parameter_info
void terminal_init();                                 //  инициализация начальными данными структуры parameter_info


#endif