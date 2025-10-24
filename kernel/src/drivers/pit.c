#include "include/drivers/pit.h"
#include "include/drivers/io.h"
#include "include/drivers/serial.h"
#include "libc/string.h" 
#include "include/graphics/fb.h"
#include "include/graphics/font.h"
#include "include/graphics/color.h"
#include <limine.h>

volatile uint64_t pit_ticks = 0;

void pit_init(void) {
    serial_puts("[PIT] Initializing PIT...\n");
    
    // Устанавливаем частоту
    pit_set_frequency(PIT_DIVIDER);
    
    serial_puts("[PIT] PIT initialized at 1000 Hz\n");
}

void pit_set_frequency(uint32_t divider) {
    uint32_t divisor = PIT_BASE_FREQ / divider;
    
    // Отправляем команду PIT
    outb(PIT_COMMAND, PIT_CHANNEL0_SEL | PIT_ACCESS_LOHI | PIT_MODE3);
    
    // Устанавливаем делитель
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

void pit_sleep(uint64_t ms) {
    uint64_t target_ticks = pit_ticks + ms;
    while (pit_ticks < target_ticks) {
        asm volatile("hlt");
    }
}

// Обработчик прерывания таймера
void pit_timer_handler(struct registers *regs) {
    pit_ticks++;
    // Каждую секунду выводим сообщение (для теста)
    if (pit_ticks % 1000 == 0) {
        char buffer[32];
        serial_puts("[PIT] Timer: ");
        serial_puts(itoa(pit_ticks / 1000, buffer, 10));
        serial_puts(" seconds\n");
    }
}