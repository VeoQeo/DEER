#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "../stdio.h"
#include "../../include/graphics/fb.h"
#include "../../include/graphics/font.h"
#include "../../include/graphics/color.h"
#include "../../include/drivers/serial.h"
#include "../string.h"
#include <limine.h>

static struct limine_framebuffer *current_fb = NULL;
static uint32_t cursor_x = 5;
static uint32_t cursor_y = 5;
static uint32_t text_color = COLOR_WHITE;
static uint32_t bg_color = COLOR_BLACK;
static int printf_initialized = 0;

static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

static void printf_auto_init(void) {
    if (printf_initialized) return;
    
    if (fb_request.response && fb_request.response->framebuffer_count > 0) {
        current_fb = fb_request.response->framebuffers[0];
        serial_puts("[PRINTF] Auto-initialized with framebuffer\n");
    } else {
        serial_puts("[PRINTF] WARNING: No framebuffer available, using serial only\n");
    }
    
    printf_initialized = 1;
}

void printf_set_framebuffer(struct limine_framebuffer *fb) {
    current_fb = fb;
    printf_initialized = 1;
    
    char buffer[32];
    serial_puts("[PRINTF] Framebuffer set manually at 0x");
    serial_puts(itoa((uint64_t)fb, buffer, 16));
    serial_puts("\n");
}

void printf_set_cursor(uint32_t x, uint32_t y) {
    cursor_x = x;
    cursor_y = y;
}

void printf_set_color(uint32_t color) {
    text_color = color;
}

void printf_set_bg_color(uint32_t color) {
    bg_color = color;
}

void printf_get_cursor(uint32_t *x, uint32_t *y) {
    *x = cursor_x;
    *y = cursor_y;
}

static void scroll_screen(void) {
    if (!current_fb) return;
    
    uint32_t scroll_height = 16; 
    uint32_t screen_height = current_fb->height;
    uint32_t screen_width = current_fb->width;
    
    for (uint32_t y = scroll_height; y < screen_height; y++) {
        for (uint32_t x = 0; x < screen_width; x++) {
            uint32_t *fb_ptr = (uint32_t *)current_fb->address;
            uint32_t pitch = current_fb->pitch / 4;
            fb_ptr[(y - scroll_height) * pitch + x] = fb_ptr[y * pitch + x];
        }
    }
    
    for (uint32_t y = screen_height - scroll_height; y < screen_height; y++) {
        for (uint32_t x = 0; x < screen_width; x++) {
            fb_draw_pixel(current_fb, x, y, bg_color);
        }
    }
    cursor_y -= scroll_height;
}

void printf_clear(void) {
    printf_auto_init();
    
    if (!current_fb) return;
    
    fb_clear(current_fb, bg_color);
    cursor_x = 5;
    cursor_y = 5;
}

static void putchar(char c) {
    printf_auto_init();
    
    if (c == '\n') {
        serial_putc('\r');
    }
    serial_putc(c);
    
    if (!current_fb) return;
    
    switch (c) {
        case '\n': 
            cursor_x = 5;
            cursor_y += 16;
            break;
            
        case '\r': 
            cursor_x = 5;
            break;
            
        case '\t': 
            cursor_x = (cursor_x + 32) & ~31; 
            break;
            
        case '\b': 
            if (cursor_x > 5) {
                cursor_x -= 8;
                fb_fill_rect(current_fb, cursor_x, cursor_y, 8, 16, bg_color);
            }
            break;
            
        default:
            if (cursor_x + 8 >= current_fb->width) {
                cursor_x = 5;
                cursor_y += 16;
            }
            
            if (cursor_y + 16 >= current_fb->height) {
                scroll_screen();
            }
            
            fb_draw_char(current_fb, c, cursor_x, cursor_y, text_color);
            cursor_x += 8;
            break;
    }
}

static void putstring(const char *str) {
    while (*str) {
        putchar(*str++);
    }
}

static void putnumber(uint64_t num, int base, int is_signed) {
    char buffer[65];
    
    if (is_signed && (int64_t)num < 0) {
        putchar('-');
        num = (uint64_t)(-(int64_t)num);
    }
    
    itoa(num, buffer, base);
    
    putstring(buffer);
}

int printf(const char *format, ...) {
    printf_auto_init();
    
    va_list args;
    va_start(args, format);
    
    int chars_written = 0;
    
    while (*format) {
        if (*format == '%') {
            format++;
            
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

            switch (*format) {
                case 'd':
                case 'i': { 
                    if (is_long) {
                        uint64_t num = (uint64_t)va_arg(args, long long);
                        putnumber(num, 10, 1);
                    } else {
                        uint32_t num = (uint32_t)va_arg(args, int);
                        putnumber(num, 10, 1);
                    }
                    chars_written++;
                    break;
                }
                
                case 'u': { 
                    if (is_long) {
                        uint64_t num = (uint64_t)va_arg(args, unsigned long long);
                        putnumber(num, 10, 0);
                    } else {
                        uint32_t num = (uint32_t)va_arg(args, unsigned int);
                        putnumber(num, 10, 0);
                    }
                    chars_written++;
                    break;
                }
                
                case 'x': { 
                    if (is_long) {
                        uint64_t num = (uint64_t)va_arg(args, unsigned long long);
                        if (is_alt) putstring("0x");
                        putnumber(num, 16, 0);
                    } else {
                        uint32_t num = (uint32_t)va_arg(args, unsigned int);
                        if (is_alt) putstring("0x");
                        putnumber(num, 16, 0);
                    }
                    chars_written++;
                    break;
                }
                
                case 'o': { 
                    if (is_long) {
                        uint64_t num = (uint64_t)va_arg(args, unsigned long long);
                        if (is_alt) putstring("0");
                        putnumber(num, 8, 0);
                    } else {
                        uint32_t num = (uint32_t)va_arg(args, unsigned int);
                        if (is_alt) putstring("0");
                        putnumber(num, 8, 0);
                    }
                    chars_written++;
                    break;
                }
                
                case 'p': { 
                    void *ptr = va_arg(args, void*);
                    putstring("0x");
                    putnumber((uint64_t)ptr, 16, 0);
                    chars_written++;
                    break;
                }
                
                case 'c': { 
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    chars_written++;
                    break;
                }
                
                case 's': { 
                    char *str = va_arg(args, char*);
                    if (!str) {
                        putstring("(null)");
                    } else {
                        putstring(str);
                    }
                    chars_written++;
                    break;
                }
                
                case '%': { 
                    putchar('%');
                    chars_written++;
                    break;
                }
                
                default: 
                    putchar('%');
                    putchar(*format);
                    chars_written += 2;
                    break;
            }
        } else {
            putchar(*format);
            chars_written++;
        }
        
        format++;
    }
    
    va_end(args);
    return chars_written;
}

int puts(const char *str) {
    int result = printf("%s\n", str);
    return result;
}

void printf_init_with_framebuffer(struct limine_framebuffer *fb) {
    current_fb = fb;
    printf_initialized = 1;
    serial_puts("[PRINTF] Initialized with framebuffer\n");
}

// Вывод uint64_t
void printf_uint64(uint64_t value) {
    char buffer[21]; 
    itoa(value, buffer, 10);
    printf("%s", buffer);
}