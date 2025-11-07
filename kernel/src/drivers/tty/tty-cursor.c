#include <stdint.h>

#include "../../include/drivers/tty/main-tty-struct.h"
#include "../../include/drivers/tty/tty-cursor.h"
#include "../../include/drivers/io.h"


// ПОЛУЧИТЬ ТЕКУЩИЕ КООРДИНАТЫ (ЛОГИЧЕСКИЕ: ОТНОСИТЕЛЬНО БЕКЛОГА А НЕ ВИДИМОГО ЭКРАНА) 
current_coordinates get_cursor_position() {
    struct tty_parameters *parameter_info = interact_tty_parameters();  //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ TTY СТРУКТУРЕ 

    current_coordinates coordinate = { 
        .x = parameter_info -> cursor_position.x, 
        .y = parameter_info -> cursor_position.y 

    };

    return coordinate;
}

// ОБНОВИТЬ ПОЛОЖЕНИЕ КУРСОРА 
void update_cursor_position(int y, int x) { 
    struct tty_parameters *parameter_info = interact_tty_parameters();  //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ TTY СТРУКТУРЕ 
            
    // 1. Обновляем актуальные координаты в структуре
    parameter_info -> cursor_position.x = x; 
    parameter_info -> cursor_position.y = y; 

    // 2. Находим линейную координату для VGA-контроллера. [Формула]: <строка> * <ширина экрана> + <столбец>
    uint16_t current_position = y * VGA_OUTPUT_LENGTH + x; 

    // 3. Присваиваем старший байт позиции курсора (регистр 0x0E)
    outb(parameter_info -> cursor_position.io_index_port, CRTC_REGISTER_CURSOR_HIGH_POSITION); 
    outb(parameter_info -> cursor_position.io_data_port, (current_position >> 8) & 0xFF);

    // 4. Присваиваем младший байт позиции курсора (регистр 0x0F)
    outb(parameter_info -> cursor_position.io_index_port, CRTC_REGISTER_CURSOR_LOW_POSITION); 
    outb(parameter_info -> cursor_position.io_data_port, current_position & 0xFF);

}

// ФОРМА КУРСОРА И ЕГО МИГАНИЕ --> [ЗАГЛУШКА]: ПО УМОЛЧАНИЮ ПОДЧЕРКИВАЕТ ТЕК. СИМВОЛ
void set_cursor_shape() {

    outb(IO_INDEX_PORT_COLOR, CRTC_REGISTER_CURSOR_START_LINE);

    uint8_t current_value = inb(IO_DATA_PORT_COLOR);
    current_value = (current_value & 0xC0) | CURSOR_BLINK_ENABLE_BIT | CURSOR_SHAPE_UNDERLINE_START_VALUE;    

    out(IO_DATA_PORT_COLOR, current_value);
    outb(IO_INDEX_PORT_COLOR, CRTC_REGISTER_CURSOR_END_LINE);
    outb(IO_DATA_PORT_COLOR, CURSOR_SHAPE_UNDERLINE_END_VALUE);

}

/*
// Ф-Я ДЛЯ ДИНАМИЧЕСКОЙ УСТАНОВКИ ПОВЕДЕНИЯ КУРСОРА 
void set_cursor_shape(uint8_t start_line, uint8_t end_line, bool blink_enable, bool visible) {
    struct tty_parameters *parameter_info = interact_tty_parameters();
    
    pthread_mutex_lock(&parameter_info->tty_cursor_lock);

    // 1. Обновляем поля в структуре cursor_shape с НОВЫМИ значениями
    parameter_info->cursor_shape.pixel_start_line = start_line;
    parameter_info->cursor_shape.pixel_end_line   = end_line;
    parameter_info->cursor_shape.isBlink_enable   = blink_enable;
    parameter_info->cursor_shape.isVisible        = visible;

    // 2. Используем ОБНОВЛЕННЫЕ данные из структуры для настройки аппаратного курсора VGA

    // Настройка регистра 0x0A (CRTC_REGISTER_CURSOR_START_LINE)
    outb(parameter_info->cursor_position.io_index_port, CRTC_REGISTER_CURSOR_START_LINE);
    uint8_t current_reg_val = inb(parameter_info->cursor_position.io_data_port);

    uint8_t new_reg_val = (current_reg_val & 0xC0); // Сохраняем верхние биты
    if (parameter_info->cursor_shape.isBlink_enable) {
        new_reg_val |= CURSOR_BLINK_ENABLE_BIT;
    }
    if (!parameter_info->cursor_shape.isVisible) { // Если курсор должен быть скрыт
        new_reg_val |= CURSOR_DISABLE_BIT; // Бит 6 для полного выключения курсора
    }
    new_reg_val |= parameter_info->cursor_shape.pixel_start_line; // Начальная строка

    outb(parameter_info->cursor_position.io_data_port, new_reg_val);

    // Настройка регистра 0x0B (CRTC_REGISTER_CURSOR_END_LINE)
    outb(parameter_info->cursor_position.io_index_port, CRTC_REGISTER_CURSOR_END_LINE);
    outb(parameter_info->cursor_position.io_data_port, parameter_info->cursor_shape.pixel_end_line);
    
    pthread_mutex_unlock(&parameter_info->tty_cursor_lock);
}
*/