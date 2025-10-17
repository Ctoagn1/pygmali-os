#include <stdint.h>
#define IDT_ENTRIES 256
#include "keyboardhandler.h"
#include "idt.h"
#include "tty.h"
#include "rtc.h"
#include "pit.h"
#include "printf.h"

typedef struct{
    uint16_t first_offset;
    uint16_t segment_selector;
    uint16_t weird_bytes;
    uint16_t middle_offset;
    uint32_t ending_offset;
    uint32_t reserved;
} IDT_Entry;
IDT_Entry idt[IDT_ENTRIES];
typedef struct{
    uint32_t cases, edi, esi, ebp, esp, ebx, edx, ecx, eax; //pushed by pushad in wrapper
} ExceptionStack;


void makeIDTEntry(IDT_Entry *entry, uint64_t offset, uint16_t selector, uint8_t gate, uint8_t dpl){
    //gates- 0101 is task gate- in task gates offset should be 0,
    //0110 is 16 bit interrupt gate, 0111 is 16 bit trap gate. trap gates allow further interrupts when one is being handled
    //1110 is 32 bit interrupt, 1111 is 32 bit trap. dpl is privilege, 0 being kernel 3 being user
    entry->first_offset=(offset&0xFFFF);
    entry->segment_selector=selector;
    entry->middle_offset=((offset>>16)&0xFF);
    uint16_t weird_bytes=(1<<15)|(gate<<8)|(dpl<<13);
    entry->weird_bytes=weird_bytes;
    entry->ending_offset=offset>>32;
}

void initIdt(){
    makeIDTEntry(&idt[33], (uint64_t)&write_to_buffer_wrapper, 0x08, 0b1110, 0); //keyboard
    makeIDTEntry(&idt[40], (uint64_t)&update_time_wrapper, 0x08, 0b1110, 0); //real time clock
    makeIDTEntry(&idt[32], (uint64_t)&pit_timer_wrapper, 0x08, 0b1110, 0); //programmable interval timer
    makeIDTEntry(&idt[0], (uint64_t)&exception_0_wrapper, 0x08, 0b1110, 0); 
    makeIDTEntry(&idt[1], (uint64_t)&exception_1_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[2], (uint64_t)&exception_2_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[3], (uint64_t)&exception_3_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[4], (uint64_t)&exception_4_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[5], (uint64_t)&exception_5_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[6], (uint64_t)&exception_6_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[7], (uint64_t)&exception_7_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[8], (uint64_t)&exception_8_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[9], (uint64_t)&exception_9_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[10], (uint64_t)&exception_10_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[11], (uint64_t)&exception_11_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[12], (uint64_t)&exception_12_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[13], (uint64_t)&exception_13_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[14], (uint64_t)&exception_14_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[16], (uint64_t)&exception_16_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[17], (uint64_t)&exception_17_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[18], (uint64_t)&exception_18_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[19], (uint64_t)&exception_19_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[20], (uint64_t)&exception_20_wrapper, 0x08, 0b1110, 0);
    makeIDTEntry(&idt[21], (uint64_t)&exception_21_wrapper, 0x08, 0b1110, 0);


    reloadIDT(sizeof(idt)-1, (uint32_t)&idt);
}
void panic(char *error){
    disable_cursor();
	printf("FATAL ERROR %s\nSYSTEM HALTED", error);
	 __asm__ volatile("cli; hlt"); //disable interrupts, halt cpu function
}
void print_regs(ExceptionStack *regs){
    printf("REGISTER DUMP\nEAX: %08x\nECX: %08x\nEDX: %08x\nEBX: %08x\nESP: %08x\nEBP: %08x\nESI: %08x\nEDI: %08x\n", regs->eax, regs->ecx, regs->edx, regs->ebx, regs->esp, regs->ebp, regs->esi, regs->edi);
}
void page_fault_display(uint32_t page, uint32_t errcode){
    errcode &= 0b111;
    printf("ERROR WITH PAGE %08x\n", page);
    switch (errcode){
        case 0:
            printf("SUPERVISOR PROCESS READ NON PRESENT PAGE ENTRY\n");
            break;
        case 1:
            printf("SUPERVISOR PROCESS READ PAGE, CAUSED PROTECTION FAULT\n");
            break;
        case 2:
            printf("SUPERVISOR PROCESS WROTE NON PRESENT PAGE ENTRY\n");
            break;
        case 3:
            printf("SUPERVISOR PROCESS WROTE PAGE, CAUSED PROTECTION FAULT\n");
            break;
        case 4:
            printf("USER PROCESS READ NON PRESENT PAGE ENTRY\n");
            break;
        case 5:
            printf("USER PROCESS READ PAGE, CAUSED PROTECTION FAULT\n");
            break;
        case 6:
            printf("USER PROCESS WROTE NON PRESENT PAGE ENTRY\n");
            break;
        case 7:
            printf("USER PROCESS WROTE PAGE, CAUSED PROTECTION FAULT\n");
            break;
    }
}
void exception_handler(ExceptionStack *regs){
    print_regs(regs);
    switch (regs->cases){
        case 0:
            panic("DIVIDE BY ZERO");
            break;
        case 1:
            panic("DEBUG");
            break;
        case 2:
            panic("NONMASKABLE INTERRUPT");
            break;
        case 3:
            panic("BREAKPOINT");
            break;
        case 4:
            panic("OVERFLOW");
            break;
        case 5:
            panic("BOUND RANGE EXCEEDED");
            break;
        case 6:
            panic("INVALID OPCODE");
            break;
        case 7:
            panic("DEVICE NOT AVAILABLE");
            break;
        case 8:
            panic("DOUBLE FALT");
            break;
        case 9:
            panic("COPROCESSOR SEGMENT OVERRUN");
            break;
        case 10:
            panic("INVALID TSS");
            break;
        case 11:
            panic("SEGMENT NOT PRESENT");
            break;
        case 12:
            panic("STACK-SEGMENT FAULT");
            break;
        case 13:
            panic("GENERAL PROTECTION FAULT");
            break;
        case 14:
            panic("PAGE FAULT");
            break;
        //case 15 reserved
        case 16:
            panic("x87 FPU FLOATING POINT ERROR");
            break;
        case 17:
            panic("ALIGNMENT CHECK");
            break;
        case 18:
            panic("MACHINE CHECK");
            break;
        case 19:
            panic("SIMD FLOATING POINT EXCEPTION");
            break;
        case 20:
            panic("VIRTUALIZATION EXCEPTION");
            break;
        case 21:
            panic("CONTROL PROTECTION EXCEPTION");
            break;
    }
}
