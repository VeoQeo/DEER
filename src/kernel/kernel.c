#include <stdint.h>

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define WHITE_ON_BLACK 0x0F
#define RED_ON_BLACK 0x0C
#define GREEN_ON_BLACK 0x0A

// Функция очистки экрана
void clear_screen() {
    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    uint16_t blank = ' ' | (WHITE_ON_BLACK << 8);

    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = blank;
    }
}

void kernel_main(void) {
    clear_screen();

    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    vga[0] = 'A' | (RED_ON_BLACK << 8);
    vga[1] = 'B' | (GREEN_ON_BLACK << 8);
    vga[2] = 'C' | (WHITE_ON_BLACK << 8);
	vga[3] = ' ' | (WHITE_ON_BLACK << 8);
    vga[4] = '6' | (WHITE_ON_BLACK << 8);
    vga[5] = '4' | (WHITE_ON_BLACK << 8);
	vga[6] = ' ' | (WHITE_ON_BLACK << 8);
    vga[7] = 'B' | (WHITE_ON_BLACK << 8);
    vga[8] = 'I' | (WHITE_ON_BLACK << 8);
	vga[9] = 'N' | (WHITE_ON_BLACK << 8);

    while(1); // чтобы QEMU не закрывался
}