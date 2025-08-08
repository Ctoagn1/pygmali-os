#ifndef IDT_INIT
#define IDT_INIT
#include <stdint.h>
void reloadIDT(uint16_t size, uint32_t offset);
void initIdt();
void panic(char *error);
#endif
void exception_0_wrapper();
void exception_1_wrapper();
void exception_2_wrapper();
void exception_3_wrapper();
void exception_4_wrapper();
void exception_5_wrapper();
void exception_6_wrapper();
void exception_7_wrapper();
void exception_8_wrapper();
void exception_9_wrapper();
void exception_10_wrapper();
void exception_11_wrapper();
void exception_12_wrapper();
void exception_13_wrapper();
void exception_14_wrapper();
void exception_16_wrapper();
void exception_17_wrapper();
void exception_18_wrapper();
void exception_19_wrapper();
void exception_20_wrapper();
void exception_21_wrapper();