#ifndef PIT_H
#define PIT_H

#include <stdint.h>
#include "../interrupts/isr.h"

#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

// Команды PIT
#define PIT_CHANNEL0_SEL    0x00
#define PIT_ACCESS_LOHI     0x30    
#define PIT_MODE3           0x06    

// Частота PIT
#define PIT_BASE_FREQ       1193182
#define PIT_DIVIDER         1000    // 1000 Hz = 1 мс

extern volatile uint64_t pit_ticks;

void pit_init(void);
void pit_set_frequency(uint32_t freq);
uint64_t pit_get_ticks(void);
void pit_sleep(uint64_t ms);
void pit_timer_handler(struct registers *regs);

#endif // PIT_H