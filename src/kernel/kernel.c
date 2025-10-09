// src/kernel/kernel.c
#include <stdint.h>

// === VGA Text Mode ===
static int vga_col = 0;
static int vga_row = 0;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000

// Цвета
#define WHITE_ON_BLACK 0x0F
#define RED_ON_BLACK   0x0C
#define GREEN_ON_BLACK 0x0A
#define BLUE_ON_BLACK  0x09
#define YELLOW_ON_BLACK 0x0E

// Простая itoa (только для положительных чисел)
void itoa(uint64_t n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    buf[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
}

// Установка символа с цветом
void vga_putentryat(char c, uint8_t color, int col, int row) {
    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    int index = row * VGA_WIDTH + col;
    vga[index] = (uint16_t)(c | (color << 8));
}

// Прокрутка экрана
void vga_scroll() {
    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    if (vga_row >= VGA_HEIGHT) {
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            vga[i] = vga[i + VGA_WIDTH];
        }
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            vga[i] = ' ' | (WHITE_ON_BLACK << 8);
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

// Вывод строки с поддержкой \n и цветов
void vga_writex(const char* str, uint8_t color) {
    while (*str) {
        if (*str == '\n') {
            vga_row++;
            vga_col = 0;
        } else {
            vga_putentryat(*str, color, vga_col, vga_row);
            vga_col++;
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
        }
        str++;
        vga_scroll();
    }
}

// Удобная функция для вывода с цветом
void vga_write(const char* str) {
    vga_writex(str, WHITE_ON_BLACK);
}

void vga_error(const char* str) {
    vga_writex(str, RED_ON_BLACK);
}

void vga_success(const char* str) {
    vga_writex(str, GREEN_ON_BLACK);
}

// === Главная функция ядра ===
void kernel_main() {
    // Очистка экрана
    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = ' ' | (WHITE_ON_BLACK << 8);
    }

    vga_success("DEER OS v0.0.1\n");
    vga_write("====================\n");


    while(1);
}