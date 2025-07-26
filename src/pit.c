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
#include "note_definitions.h"
uint64_t ms_timer=0;
Note note_table[] = {
    {33, "C1"}, {35, "CSHARP1"}, {37, "D1"}, {39, "DSHARP1"},
    {41, "E1"}, {44, "F1"}, {46, "FSHARP1"}, {49, "G1"}, {52, "GSHARP1"},
    {55, "A1"}, {58, "ASHARP1"}, {62, "B1"},
    
    {65, "C2"}, {69, "CSHARP2"}, {73, "D2"}, {78, "DSHARP2"},
    {82, "E2"}, {87, "F2"}, {93, "FSHARP2"}, {98, "G2"}, {104, "GSHARP2"},
    {110, "A2"}, {117, "ASHARP2"}, {123, "B2"},
    
    {131, "C3"}, {139, "CSHARP3"}, {147, "D3"}, {156, "DSHARP3"},
    {165, "E3"}, {175, "F3"}, {185, "FSHARP3"}, {196, "G3"}, {208, "GSHARP3"},
    {220, "A3"}, {233, "ASHARP3"}, {247, "B3"},
    
    {262, "C4"}, {277, "CSHARP4"}, {294, "D4"}, {311, "DSHARP4"},
    {330, "E4"}, {349, "F4"}, {370, "FSHARP4"}, {392, "G4"}, {415, "GSHARP4"},
    {440, "A4"}, {466, "ASHARP4"}, {494, "B4"},
    
    {523, "C5"}, {554, "CSHARP5"}, {587, "D5"}, {622, "DSHARP5"},
    {659, "E5"}, {698, "F5"}, {740, "FSHARP5"}, {784, "G5"}, {831, "GSHARP5"},
    {880, "A5"}, {932, "ASHARP5"}, {988, "B5"},
    
    {1047, "C6"}, {1109, "CSHARP6"}, {1175, "D6"}, {1245, "DSHARP6"},
    {1319, "E6"}, {1397, "F6"}, {1480, "FSHARP6"}, {1568, "G6"}, {1661, "GSHARP6"},
    {1760, "A6"}, {1865, "ASHARP6"}, {1976, "B6"}
};
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
    play_sound(D4, 200);
	play_sound(D4, 200);
	play_sound(D5, 200);
    msleep(100);
    play_sound(A4, 200);
	msleep(200);
	play_sound(GSHARP4, 200);
	msleep(50);
	play_sound(G4, 200);
	msleep(75);
	play_sound(F4, 200);
}
