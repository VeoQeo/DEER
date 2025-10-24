#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "../stdio.h"
#include "../../include/graphics/fb.h"
#include "../../include/graphics/font.h"
#include "../../include/graphics/color.h"
#include "../../include/drivers/serial.h"
#include "../string.h"

// Глобальные переменные для текущего фреймбуфера и позиции курсора
static struct limine_framebuffer *current_fb = NULL;
static uint32_t cursor_x = 5;
static uint32_t cursor_y = 5;
static uint32_t text_color = COLOR_WHITE;
static uint32_t bg_color = COLOR_BLACK;

// Установка текущего фреймбуфера
void printf_set_framebuffer(struct limine_framebuffer *fb) {
    current_fb = fb;
}

// Установка позиции курсора
void printf_set_cursor(uint32_t x, uint32_t y) {
    cursor_x = x;
    cursor_y = y;
}

// Установка цвета текста
void printf_set_color(uint32_t color) {
    text_color = color;
}

// Установка цвета фона
void printf_set_bg_color(uint32_t color) {
    bg_color = color;
}

// Получение текущей позиции курсора
void printf_get_cursor(uint32_t *x, uint32_t *y) {
    *x = cursor_x;
    *y = cursor_y;
}

// Прокрутка экрана
static void scroll_screen(void) {
    if (!current_fb) return;
    
    uint32_t scroll_height = 16; // Высота одной строки
    uint32_t screen_height = current_fb->height;
    uint32_t screen_width = current_fb->width;
    
    // Копируем содержимое экрана вверх
    for (uint32_t y = scroll_height; y < screen_height; y++) {
        for (uint32_t x = 0; x < screen_width; x++) {
            uint32_t *fb_ptr = (uint32_t *)current_fb->address;
            uint32_t pitch = current_fb->pitch / 4;
            fb_ptr[(y - scroll_height) * pitch + x] = fb_ptr[y * pitch + x];
        }
    }
    
    // Очищаем последнюю строку
    for (uint32_t y = screen_height - scroll_height; y < screen_height; y++) {
        for (uint32_t x = 0; x < screen_width; x++) {
            fb_draw_pixel(current_fb, x, y, bg_color);
        }
    }
    
    // Обновляем позицию курсора
    cursor_y -= scroll_height;
}

// Очистка экрана
void printf_clear(void) {
    if (!current_fb) return;
    
    fb_clear(current_fb, bg_color);
    cursor_x = 5;
    cursor_y = 5;
}

// Вывод одного символа с обработкой специальных символов
static void putchar(char c) {
    if (!current_fb) return;
    
    switch (c) {
        case '\n': // Новая строка
            cursor_x = 5;
            cursor_y += 16;
            break;
            
        case '\r': // Возврат каретки
            cursor_x = 5;
            break;
            
        case '\t': // Табуляция
            cursor_x = (cursor_x + 32) & ~31; // Выравнивание до 32 пикселей
            break;
            
        case '\b': // Backspace
            if (cursor_x > 5) {
                cursor_x -= 8;
                // Стираем символ
                fb_fill_rect(current_fb, cursor_x, cursor_y, 8, 16, bg_color);
            }
            break;
            
        default:
            // Проверяем, не вышли ли мы за границы экрана
            if (cursor_x + 8 >= current_fb->width) {
                cursor_x = 5;
                cursor_y += 16;
            }
            
            if (cursor_y + 16 >= current_fb->height) {
                scroll_screen();
            }
            
            // Выводим символ
            fb_draw_char(current_fb, c, cursor_x, cursor_y, text_color);
            cursor_x += 8;
            break;
    }
}

// Вспомогательная функция для вывода строки
static void putstring(const char *str) {
    while (*str) {
        putchar(*str++);
    }
}

// Вспомогательная функция для вывода числа в указанной системе счисления
static void putnumber(uint64_t num, int base, int is_signed, int is_long) {
    char buffer[65];
    char *ptr = buffer;
    
    if (is_signed && (int64_t)num < 0) {
        putchar('-');
        num = (uint64_t)(-(int64_t)num);
    }
    
    // Преобразуем число в строку
    itoa(num, buffer, base);
    
    // Выводим строку
    putstring(buffer);
}

// Основная функция printf
int printf(const char *format, ...) {
    if (!current_fb) {
        // Если фреймбуфер не установлен, выводим в serial
        serial_puts("[PRINTF] Framebuffer not set, using serial output: ");
        serial_puts(format);
        serial_puts("\n");
        return 0;
    }
    
    va_list args;
    va_start(args, format);
    
    int chars_written = 0;
    
    while (*format) {
        if (*format == '%') {
            format++;
            
            // Обработка флагов
            int is_long = 0;
            int is_alt = 0;
            
            while (1) {
                switch (*format) {
                    case 'l':
                        is_long = 1;
                        format++;
                        continue;
                    case '#':
                        is_alt = 1;
                        format++;
                        continue;
                    default:
                        break;
                }
                break;
            }
            
            // Обработка спецификаторов
            switch (*format) {
                case 'd':
                case 'i': { // Целое число со знаком
                    if (is_long) {
                        int64_t num = va_arg(args, int64_t);
                        putnumber((uint64_t)num, 10, 1, is_long);
                    } else {
                        int num = va_arg(args, int);
                        putnumber((uint64_t)num, 10, 1, is_long);
                    }
                    chars_written++;
                    break;
                }
                
                case 'u': { // Целое число без знака
                    if (is_long) {
                        uint64_t num = va_arg(args, uint64_t);
                        putnumber(num, 10, 0, is_long);
                    } else {
                        unsigned int num = va_arg(args, unsigned int);
                        putnumber(num, 10, 0, is_long);
                    }
                    chars_written++;
                    break;
                }
                
                case 'x': { // Шестнадцатеричное число
                    if (is_long) {
                        uint64_t num = va_arg(args, uint64_t);
                        if (is_alt) putstring("0x");
                        putnumber(num, 16, 0, is_long);
                    } else {
                        unsigned int num = va_arg(args, unsigned int);
                        if (is_alt) putstring("0x");
                        putnumber(num, 16, 0, is_long);
                    }
                    chars_written++;
                    break;
                }
                
                case 'o': { // Восьмеричное число
                    if (is_long) {
                        uint64_t num = va_arg(args, uint64_t);
                        if (is_alt) putstring("0");
                        putnumber(num, 8, 0, is_long);
                    } else {
                        unsigned int num = va_arg(args, unsigned int);
                        if (is_alt) putstring("0");
                        putnumber(num, 8, 0, is_long);
                    }
                    chars_written++;
                    break;
                }
                
                case 'p': { // Указатель
                    void *ptr = va_arg(args, void*);
                    putstring("0x");
                    putnumber((uint64_t)ptr, 16, 0, 1);
                    chars_written++;
                    break;
                }
                
                case 'c': { // Символ
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    chars_written++;
                    break;
                }
                
                case 's': { // Строка
                    char *str = va_arg(args, char*);
                    if (!str) {
                        putstring("(null)");
                    } else {
                        putstring(str);
                    }
                    chars_written++;
                    break;
                }
                
                case '%': { // Символ процента
                    putchar('%');
                    chars_written++;
                    break;
                }
                
                default: // Неизвестный спецификатор
                    putchar('%');
                    putchar(*format);
                    chars_written += 2;
                    break;
            }
        } else {
            // Обычный символ
            putchar(*format);
            chars_written++;
        }
        
        format++;
    }
    
    va_end(args);
    return chars_written;
}

// Упрощенная версия printf для строки
int puts(const char *str) {
    return printf("%s\n", str);
}