/* 
#include "main-tty-struct.h"

// ПРОВЕРИТЬ VGA КОНТРОЛЛЕР + ИНИЦИАЛИЗАЦИЯ ТИПА VGA (МОНОХРОМ/ЦВЕТНОЙ)
void checkout_vga(struct tty_parameters *parameter_info) { 
    uint8_t isError = 0;        // флаг ошибки
    
    // ~Переменные для CRTC-теста
    uint8_t original_crtc_value;    
    uint8_t test_crtc_value = 0xAA; 

    // ~Переменные для теста видеопамяти
    uint16_t original_vga_mem_value; 
    uint16_t test_memory_value = 0xABCD; 

    // 1. Проверяем тип контроллера по I/O адресу 0x3CC 
    uint8_t vga_type_value = inb(MOR_PORT);    // проверяем какой тип контроллера установлен (0 - монохром, 1 - цветной)
    if (vga_type_value & 0x01) {
        parameter_info -> vga_memory_address  = VGA_MEMORY_ADDRESS_COLOR;  // выставляем тип контроллера цветной 
        parameter_info -> vga_crtc_index_port = VGA_CRTC_INDEX_PORT_COLOR; 
        parameter_info -> vga_crtc_data_port  = VGA_CRTC_DATA_PORT_COLOR;
        
        // printf("[vga]: Цветной VGA-контроллер инициализирован");

    } else {
        parameter_info -> vga_memory_address  = VGA_MEMORY_ADDRESS_MONO;   // выставляем тип контроллера монохром  
        parameter_info -> vga_crtc_index_port = VGA_CRTC_INDEX_PORT_MONO;
        parameter_info -> vga_crtc_data_port  = VGA_CRTC_DATA_PORT_MONO;

        // printf("[vga]: Инициализирован монохромный VGA-контроллер");
    }

    // 2. Проверяем отклик CRTC-контроллера 
    outb(VGA_CRTC_HORIZONTAL_TOTAL_REGISTER, parameter_info -> vga_crtc_index_port); 
    original_crtc_value = inb(parameter_info -> vga_crtc_data_port); 

    outb(VGA_CRTC_HORIZONTAL_TOTAL_REGISTER, parameter_info -> vga_crtc_index_port); 
    outb(test_crtc_value, parameter_info -> vga_crtc_data_port); 

    if (inb(parameter_info->vga_crtc_data_port) == test_crtc_value) {
        // printf("[vga]: CRTC-контроллер подает сигнал\n");

    } else {
        // printf("[vga]: ОШИБКА! CRTC-контроллер НЕ подает сигнал. Ожидали 0x%x, получили 0x%x\n", test_crtc_value, inb(parameter_info -> vga_crtc_data_port)); 
        isError = 1; 
        
        // ~обработка 
    }

    outb(VGA_CRTC_HORIZONTAL_TOTAL_REGISTER, parameter_info -> vga_crtc_index_port); 
    outb(original_crtc_value, parameter_info -> vga_crtc_data_port); 

    // 3. R/W-тест видеопамяти 
    original_vga_mem_value = parameter_info -> vga_memory_address[0];       
    parameter_info -> vga_memory_address[0] = test_memory_value;           

    if (parameter_info -> vga_memory_address[0] != test_memory_value) {  
        isError = 1;
    }

    parameter_info -> vga_memory_address[0] = original_vga_mem_value;       // восстанавливаем исходное значение

    if (!isError) {
        // printf("[vga]: Доступ к видеопамяти подтвержден\n");

    } else {
        // printf("[vga]: ОШИБКА! Доступ к видеопамяти невозможен\n");
        isError = 1; 

        // ~обработка 
    }
}
*/