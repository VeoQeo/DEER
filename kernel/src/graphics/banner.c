#include <stdint.h>
#include <limine.h>
#include "../include/graphics/fb.h"
#include "../include/graphics/font.h"
#include "../include/graphics/banner.h"

void draw_deer_banner(struct limine_framebuffer *fb, uint32_t x, uint32_t y, uint32_t color) {
    const char *banner_lines[] = {
        "                                                                                   ",
        "                                                                                   ",
        "DDDDDDDDDDDDD      EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRR   ",
        "D::::::::::::DDD   E::::::::::::::::::::EE::::::::::::::::::::ER::::::::::::::::R  ",
        "D:::::::::::::::DD E::::::::::::::::::::EE::::::::::::::::::::ER::::::RRRRRR:::::R ",
        "DDD:::::DDDDD:::::DEE::::::EEEEEEEEE::::EEE::::::EEEEEEEEE::::ERR:::::R     R:::::R",
        "  D:::::D    D:::::D E:::::E       EEEEEE  E:::::E       EEEEEE  R::::R     R:::::R",
        "  D:::::D     D:::::DE:::::E               E:::::E               R::::R     R:::::R",
        "  D:::::D     D:::::DE::::::EEEEEEEEEE     E::::::EEEEEEEEEE     R::::RRRRRR:::::R ",
        "  D:::::D     D:::::DE:::::::::::::::E     E:::::::::::::::E     R:::::::::::::RR  ",
        "  D:::::D     D:::::DE:::::::::::::::E     E:::::::::::::::E     R::::RRRRRR:::::R ",
        "  D:::::D     D:::::DE::::::EEEEEEEEEE     E::::::EEEEEEEEEE     R::::R     R:::::R",
        "  D:::::D     D:::::DE:::::E               E:::::E               R::::R     R:::::R",
        "  D:::::D    D:::::D E:::::E       EEEEEE  E:::::E       EEEEEE  R::::R     R:::::R",
        "DDD:::::DDDDD:::::DEE::::::EEEEEEEE:::::EEE::::::EEEEEEEE:::::ERR:::::R     R:::::R",
        "D:::::::::::::::DD E::::::::::::::::::::EE::::::::::::::::::::ER::::::R     R:::::R",
        "D::::::::::::DDD   E::::::::::::::::::::EE::::::::::::::::::::ER::::::R     R:::::R",
        "DDDDDDDDDDDDD      EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERRRRRRRR     RRRRRRR",
        "                                                                                   ",
        "                                                                                   ",
        "                                                                                   ",
        "                                                                                   ",
        "                                                                                   ",
        "                                                                                   ",
        "                                                                                   "
    };
    
    uint32_t current_y = y;
    for (int i = 0; i < 25; i++) {
        fb_draw_string(fb, banner_lines[i], x, current_y, color);
        current_y += 16; 
    }
}