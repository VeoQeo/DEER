#include <stdint.h>

// === VGA Text Mode ===
static int vga_col = 0, vga_row = 0;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000
#define WHITE_ON_BLACK 0x0F
#define RED_ON_BLACK   0x0C
#define GREEN_ON_BLACK 0x0A

void itoa(uint64_t n, char* buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    buf[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j]; buf[j] = buf[i - 1 - j]; buf[i - 1 - j] = tmp;
    }
}

void vga_putentryat(char c, uint8_t color, int col, int row) {
    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    vga[row * VGA_WIDTH + col] = (uint16_t)(c | (color << 8));
}

void vga_scroll() {
    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    if (vga_row >= VGA_HEIGHT) {
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
            vga[i] = vga[i + VGA_WIDTH];
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++)
            vga[i] = ' ' | (WHITE_ON_BLACK << 8);
        vga_row = VGA_HEIGHT - 1;
    }
}

void vga_writex(const char* str, uint8_t color) {
    while (*str) {
        if (*str == '\n') { vga_row++; vga_col = 0; }
        else {
            vga_putentryat(*str, color, vga_col, vga_row);
            if (++vga_col >= VGA_WIDTH) { vga_col = 0; vga_row++; }
        }
        str++;
        vga_scroll();
    }
}

void vga_write(const char* str) { vga_writex(str, WHITE_ON_BLACK); }
void vga_error(const char* str) { vga_writex(str, RED_ON_BLACK); }
void vga_success(const char* str) { vga_writex(str, GREEN_ON_BLACK); }


// === Multiboot2 ===
struct mb2_tag {
    uint32_t type;
    uint32_t size;
};

struct mb2_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t red_field_pos;
    uint8_t red_mask_size;
    uint8_t green_field_pos;
    uint8_t green_mask_size;
    uint8_t blue_field_pos;
    uint8_t blue_mask_size;
    uint8_t unused;
};

#define MB2_TAG_FRAMEBUFFER 8


// === Ядро ===
void kernel_main(uint32_t magic, void* mb2_info_ptr) {
    // Очистка VGA
    volatile uint16_t* vga = (uint16_t*)VGA_ADDRESS;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = ' ' | (WHITE_ON_BLACK << 8);

    if (magic != 0x36d76289) {
        vga_error("Invalid Multiboot2 magic!\n");
        while(1);
    }

    vga_success("DEER OS v0.0.1\n");
    vga_write("====================\n");

    struct mb2_tag* tag = (struct mb2_tag*)((uint8_t*)mb2_info_ptr + 8);
    struct mb2_tag_framebuffer* fb_tag = 0;

    while (tag->type != 0) {
        if (tag->type == MB2_TAG_FRAMEBUFFER) {
            fb_tag = (struct mb2_tag_framebuffer*)tag;
            break;
        }
        uint32_t size = (tag->size + 7) & ~7;
        tag = (struct mb2_tag*)((uint8_t*)tag + size);
    }

    if (!fb_tag) {
        vga_error("Framebuffer not provided!\n");
        while(1);
    }

    if (fb_tag->bpp != 32) {
        vga_error("Only 32-bit framebuffer supported!\n");
        while(1);
    }

    uint32_t* fb = (uint32_t*)(uintptr_t)fb_tag->addr;
    uint32_t pitch = fb_tag->pitch / 4;
    uint32_t width = fb_tag->width;
    uint32_t height = fb_tag->height;

    // Очистка в чёрный
    for (uint32_t y = 0; y < height; y++)
        for (uint32_t x = 0; x < width; x++)
            fb[y * pitch + x] = 0x00000000;

    // Синий квадрат 100x100 по центру
    uint32_t sq = 100;
    uint32_t cx = width / 2 - sq / 2;
    uint32_t cy = height / 2 - sq / 2;

    for (uint32_t y = cy; y < cy + sq; y++)
        for (uint32_t x = cx; x < cx + sq; x++)
            if (x < width && y < height)
                fb[y * pitch + x] = 0xFF000000; // BGRA: Blue

    vga_success("Blue square drawn!\n");
    while(1);
}