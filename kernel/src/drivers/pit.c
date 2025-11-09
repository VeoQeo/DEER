#include "include/drivers/pit.h"
#include "include/drivers/io.h"
#include "include/drivers/serial.h"
#include "libc/string.h"
#include "libc/stdio.h"
#include "include/graphics/fb.h"
#include "include/graphics/font.h"
#include "include/graphics/color.h"
#include <limine.h>

void pit_init(void) {
    serial_puts("[PIT] Initializing PIT..."); 
    // pit_set_frequency(PIT_DIVIDER); 
    serial_puts("[PIT] PIT initialization logic moved to LAPIC timer.\n"); 
}

void pit_set_frequency(uint32_t divider) {
    (void)divider; 
    serial_puts("[PIT] pit_set_frequency is deprecated, using LAPIC timer.\n");
}

uint64_t pit_get_ticks(void) {
    extern volatile uint64_t lapic_ticks; 
    return lapic_ticks;
}

void pit_sleep(uint64_t ms) {
    extern void lapic_sleep_ms(uint64_t ms); 
    lapic_sleep_ms(ms);
}

void pit_timer_handler(struct registers *regs) {
    (void)regs;
    // pit_ticks++; 
}