//programmable interval timer
/*command register guide
Bits         Usage
7 and 6      Select channel :
                0 0 = Channel 0
                0 1 = Channel 1
                1 0 = Channel 2
                1 1 = Read-back command (8254 only)
5 and 4      Access mode :
                0 0 = Latch count value command
                0 1 = Access mode: lobyte only
                1 0 = Access mode: hibyte only
                1 1 = Access mode: lobyte/hibyte
3 to 1       Operating mode :
                0 0 0 = Mode 0 (interrupt on terminal count)
                0 0 1 = Mode 1 (hardware re-triggerable one-shot)
                0 1 0 = Mode 2 (rate generator)
                0 1 1 = Mode 3 (square wave generator)
                1 0 0 = Mode 4 (software triggered strobe)
                1 0 1 = Mode 5 (hardware triggered strobe)
                1 1 0 = Mode 2 (rate generator, same as 010b)
                1 1 1 = Mode 3 (square wave generator, same as 011b)
0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD

*/
#include "io.h"
#include <stdint.h>
#include "pic.h"
#include "pit.h"
uint64_t ms_timer=0;
void set_hertz(uint16_t hertz){ //the pit oscillates at ~1.193181666...MHz. the data bytes carry the divisor- 
    uint16_t divisor = PIT_FREQUENCY/hertz;
    uint8_t lo_divisor = (divisor & 0xFF1);
    uint8_t hi_divisor = (divisor >> 8) & 0xFF;
    outb(PIT_COMMAND_REGISTER, 0b00110100); //using mode 2, rate generator, lobyte and hibyte sent sequentially
    io_wait();
    outb(CHANNEL_0_DATA, lo_divisor);
    io_wait();
    outb(CHANNEL_0_DATA, hi_divisor);;
}
void pit_timer(){
    ms_timer++;
    PIC_sendEOI(0);
}
void sleep(uint16_t seconds){
    uint64_t duration = (uint64_t)seconds * 1000;
    uint64_t start = ms_timer;
    while((start+duration)>ms_timer){
        asm volatile("hlt");
    }
}
void msleep(uint32_t miliseconds){
    uint64_t start = ms_timer;
    while((start+miliseconds)>ms_timer){
        asm volatile("hlt");
    }
}
void play_sound(uint16_t hertz, uint32_t duration){
    uint16_t divisor = PIT_FREQUENCY/hertz;
    uint8_t lo_divisor = (divisor & 0xFF1);
    uint8_t hi_divisor = (divisor >> 8) & 0xFF;
    outb(PIT_COMMAND_REGISTER, 0b10110110); //channel 2, square wave, lobyte/hibyte
    io_wait();
    outb(CHANNEL_2_DATA, lo_divisor);
    io_wait();
    outb(CHANNEL_2_DATA, hi_divisor);;
    outb(PC_SPEAKER, 0b00000011); //link channel 2 to speaker
    msleep(duration);
    outb(PC_SPEAKER, 0);
}
void bad_time(){
    play_sound(294, 200);
	play_sound(294, 200);
	play_sound(587, 200);
    msleep(100);
    play_sound(440, 200);
	msleep(200);
	play_sound(415, 200);
	msleep(50);
	play_sound(392, 200);
	msleep(75);
	play_sound(349, 200);
}
void weezer(){
    
}