#define CHANNEL_0_DATA 0x40//generates IRQ mapped to 0, read/write
#define CHANNEL_1_DATA 0x41//irrelevant on newer machines, im including it anyways
#define CHANNEL_2_DATA 0x42 //pc speaker read/write
#define PIT_COMMAND_REGISTER 0x43 //mode/command register, write only
#define PC_SPEAKER 0x61
#define PIT_FREQUENCY 1193182
#include <stdint.h>
extern uint64_t ms_timer;
void set_hertz(uint16_t hertz);
void pit_timer_wrapper();
void sleep(uint16_t seconds);
void msleep(uint32_t miliseconds);
void play_sound(uint16_t hertz, uint32_t duration);
void bad_time();