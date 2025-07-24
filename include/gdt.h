#ifndef PINE_GDT
#define PINE_GDT

#include <stddef.h>
#include <stdint.h>

void setGDT(uint16_t size, void *base);
void reloadSegments(void);
void reloadTSS(void);
void initGdt();

#endif