#include <stdint.h>
#define IDT_ENTRIES 256
#include "keyboardhandler.h"
#include "idt.h"
#include "tty.h"
#include "rtc.h"

uint64_t idt[IDT_ENTRIES];


void makeIDTEntry(uint64_t *entry, uint32_t offset, uint16_t selector, uint8_t gate, uint8_t dpl){
    //gates- 0101 is task gate- in task gates offset should be 0,
    //0110 is 16 bit interrupt gate, 0111 is 16 bit trap gate
    //1110 is 32 bit interrupt, 1111 is 32 bit trap. dpl is privilege, 0 being kernel 3 being user
    uint64_t result = 0;
    result = offset & 0xFFFF; //split offset into first and last 16 bits
    result = result | (((uint64_t)offset >> 16) << 48);
    result = result | ((uint32_t)selector << 16); //selector goes in next 16 bits
    uint64_t weird_bytes = ((uint64_t)gate<<40) | ((uint64_t)dpl<<45) | ((uint64_t)1<<47);
    result = result | weird_bytes;
    *entry = result;
}

void initIdt(){
    makeIDTEntry(&idt[33], (uint32_t)&write_to_buffer_wrapper, 0x08, 0b1110, 0); //keyboard
    makeIDTEntry(&idt[40], (uint32_t)&update_time_wrapper, 0x08, 0b1110, 0); //real time clock
    reloadIDT(sizeof(idt)-1, (uint32_t)&idt);
    terminal_writestring("IDT loaded successfully...\n");
}