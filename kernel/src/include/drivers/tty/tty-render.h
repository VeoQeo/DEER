#ifndef TTY_RENDER_H
#define TTY_RENDER_H

#include "main-tty-struct.h"

//--------------------//
//    СПИСОК МОДОВ    //
//--------------------//

typedef enum {
/*terminal-control*/
    MODE_INIT,                 //>>обновляет всю сетку (2000 символов при размере 80х25)
    MODE_SCROLL_UP,           //>>скроллер вверх
    MODE_SCROLL_DOWN,        //>>скроллер вниз
    MODE_SET_COLOR_THEME,   //>>сменить цвет темы
    MODE_CLEAR,            //>>очистка терминала

/*getchar*/
    MODE_RETURN_CARRIEGE_INPUT,  //>>\r возврат каретки
    MODE_BACKSPACE_INPUT,       //>>бекспейс
    MODE_TAB_INPUT,            //>>табуляция
    MODE_SPACE_INPUT,         //>>пробел
    MODE_CHAR_INPUT,         //>>вывод одного символа
    MODE_ENTER_INPUT,       //>>вызов print_username

/*printf/putchar*/
    MODE_RETURN_CARRIAGE_OUTPUT,  //>>\r возврат вначало строки (перед служебным полем)
    MODE_BACKSPACE_OUTPUT,       //>>\b возврат на 1 символ назад
    MODE_TAB_OUTPUT,            //>>\t 4 пробела
    MODE_CHAR_OUTPUT,          //>>вписать 1 символ в сетку
    MODE_ENTER_OUTPUT

} set_mode_t;

//----------------------------------------//
//    СПИСОК СТРУКТУР ДЛЯ КАЖДОГО МОДА    //
//----------------------------------------//

/*MODE_INIT*/
typedef struct {
/*для service_field*/
    char *user_name;       //<<имя пользователя. ПОКА ЗАГЛУШКА
    int user_name_length; //<<фиксированная длина. ПОКА ЗАГЛУШКА

/*для backlog*/    
    uint8_t tty_symbol_color_theme;         //<<инициализируем цвета символов
    uint8_t tty_background_color_theme;    //<<инициализируем цвета фона

    // uint8_t symbol_buffer[VGA_OUTPUT_HEIGHT][VGA_OUTPUT_LENGTH];         //<<тут будем хранить стартовый вывод. Например логотип
    // uint8_t blended_color_buffer[VGA_OUTPUT_HEIGHT][VGA_OUTPUT_LENGTH]; //<<смешанный цвет стартового вывода

/*для cursor_position*/
    int y;    //<<начальная координата курсора
    int x;   //<<начальная координата курсора

    //...данные для изменения формы курсора

/*остальное*/
    bool isEnter;

    int upper_limit;     //<<верхняя граница
    int lower_limit;    //<<нижняя граница

} init_data;

/*MODE_SCROLL_DOWN*/
typedef struct {
    int lines; //>>количество строк для прокрутки вниз

} scroll_down_data;

/*MODE_SCROLL_UP*/
typedef struct {
    int lines; //>>количество строк для прокрутки вверх

} scroll_up_data;

/*MODE_SET_COLOR_THEME*/
typedef struct {
    uint8_t symbol_color_theme;
    uint8_t background_color_theme;

} set_color_theme_data;

/*MODE_CHAR_INPUT*/
typedef struct {
    uint8_t symbol;

} char_input_data;

/*MODE_ENTER_INPUT*/
typedef struct {
    bool isEnter;

} char_enter_data;

/*MODE_CHAR_OUTPUT*/
typedef struct {
    uint8_t symbol;

} char_output_data;


// -- КОНЕЦ СПИСКА -- //

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                                                         *
 *    1. ИНИЦИАЛИЗИРУЕМ ОБЪЕКТ ВНУТРИ ФУНКЦИИ (назовем FUNC)                                               *
 *    2. ОПРЕДЕЛЯЕМ МОД (СПОСОБ ЗАПИСИ) И ПАРАМЕТРЫ, КОТОРЫЕ ХОТИМ ПЕРЕДАТЬ НА ЭКРАН                       *
 *    3. ЗАПОЛНЯЕМ СТРУКТУРУ И ПЕРЕДАЕМ В TERMINAL_UPDATE                                                  *
 *    4. TERMINAL_UPDATE ВЫЗЫВАЕМ ВНУТРИ FUNC, ПЕРЕДАВАЯ ИНИЦИАЛИЗИРОВАННЫЙ ОБЪЕКТ КАК АРГУМЕНТ            *
 *                                                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// ДАННЫЕ ЗАПРОСА НА ИЗМЕНЕНИЕ
typedef struct {
    set_mode_t mode;

    // ~используем юнион для экономии места
    union {
        init_data init_data;
        scroll_up_data scroll_up;
        scroll_down_data scroll_down;
        set_color_theme_data set_color;
        char_input_data char_input;
        char_enter_data enter_input;
        char_output_data char_output;
    } params;

} update_request_t;

void terminal_clear();
void proccess_command();
void print_username(); 
void render();
void terminal_update(update_request_t *request);

#endif