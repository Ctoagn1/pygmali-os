#ifndef IDT_INIT
#define IDT_INIT
#include <stdint.h>
void reloadIDT(uint16_t size, uint32_t offset);
void initIdt();
#endif