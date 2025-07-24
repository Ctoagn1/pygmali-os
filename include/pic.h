#ifndef INIT_PIC
#define INIT_PIC
#include <stdint.h>
void PIC_remap(int offset1, int offset2);
void PIC_sendEOI(uint8_t irq);
void pic_disable(void);
uint16_t pic_get_isr(void);
uint16_t pic_get_irr(void);
void IRQ_set_mask(uint8_t IRQline);
void IRQ_clear_mask(uint8_t IRQline);
#endif