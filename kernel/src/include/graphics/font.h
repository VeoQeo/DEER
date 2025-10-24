// kernel/include/graphics/font.h
#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stddef.h>

// Структура заголовка PSFv2
struct psf_header {
    uint32_t magic;        // 0x72b54a86
    uint32_t version;      // 0
    uint32_t headersize;   // 32
    uint32_t flags;        // 0 или 1 (есть таблица Unicode)
    uint32_t numglyph;     // число глифов
    uint32_t bytesperglyph;// размер одного глифа
    uint32_t height;       // высота символа
    uint32_t width;        // ширина символа (обычно 8)
} __attribute__((packed));

// Внешние символы, созданные objcopy — объявляем как uint8_t для ясности
extern uint8_t _binary_font_psf_start[];
extern uint8_t _binary_font_psf_end[];

// Получить указатель на начало данных шрифта
static inline const uint8_t* get_font_data(void) {
    return (const uint8_t*)&_binary_font_psf_start;
}
static inline size_t get_font_data_size(void) {
    return _binary_font_psf_end - _binary_font_psf_start;
}

// Получить заголовок шрифта
static inline const struct psf_header* get_psf_header(void) {
    return (const struct psf_header*)&_binary_font_psf_start;
}

// Проверить, что шрифт корректен
int psf_validate(void) ;
// Отрисовка символа
void fb_draw_char(struct limine_framebuffer *fb, char c, uint32_t x, uint32_t y, uint32_t color);

// Отрисовка строки
void fb_draw_string(struct limine_framebuffer *fb, const char *str, uint32_t x, uint32_t y, uint32_t color);
#endif // FONT_H