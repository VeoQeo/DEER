#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limine.h>
#include "include/graphics/fb.h"
#include "include/graphics/color.h"
#include "include/graphics/font.h"
#include "include/drivers/serial.h"
#include "include/graphics/banner.h"
#include "libc/string.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

extern uint8_t _binary_font_psf_start[];
extern uint8_t _binary_font_psf_end[];

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

void kernel_main(void) {
    serial_init();
    serial_puts("[DEER] Kernel started\n");

    // Проверка шрифта
    int font_type = psf_validate();
    char buf[32];
    serial_puts("[DEER] Font type: ");
    itoa(font_type, buf, 10);
    serial_puts(buf);
    serial_puts("\n");

    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        serial_puts("[DEER] No framebuffer!\n");
        asm ("hlt");
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    
    // Информация о фреймбуфере
    serial_puts("[DEER] Framebuffer: ");
    itoa(fb->width, buf, 10); serial_puts(buf); serial_puts("x");
    itoa(fb->height, buf, 10); serial_puts(buf); serial_puts("\n");
    
    fb_clear(fb, COLOR_BLACK);
    
    // Рисуем баннер DEER в левом верхнем углу
    //draw_deer_banner(fb, 10, 10, COLOR_GREEN);  // x=10, y=10, зеленый цвет
    
    fb_draw_string(fb, "DEER OS DEV Tests:", 5, 5, COLOR_WHITE);

    while(1) {
        asm ("pause");
    }
}