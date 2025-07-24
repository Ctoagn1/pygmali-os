#define GDT_ENTRIES 6
#include <stdint.h>
#include "gdt.h"
#include "tty.h"

uint8_t gdt[GDT_ENTRIES*8];
uint16_t gdtsize = GDT_ENTRIES*8-1;

typedef struct __attribute__((packed)) TSS{
    uint16_t link;
    uint16_t res_1;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t res_2;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t res_3;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t res_4;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t res_5;
    uint16_t cs;
    uint16_t res_6;
    uint16_t ss;
    uint16_t res_7;
    uint16_t ds;
    uint16_t res_8;
    uint16_t fs;
    uint16_t res_9;
    uint16_t gs;
    uint16_t res_10;
    uint16_t ldtr;
    uint32_t res_11;
    uint16_t iopb;
    uint32_t ssp;
} TSS;

TSS tss = {0};

typedef struct GDT{
    uint32_t base;
    uint32_t limit;
    uint8_t access_byte;
    uint8_t flags;
} GDT;

void encodeGdtEntry(uint8_t *target, struct GDT source)
{
    //check limit to make sure it can't be encoded- TODO- add kernel error
    //if (source.limit > 0xFFFFF) {kerror("GDT cannot encode limits larger than 0xFFFFF");}

    //encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;

    //encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;
    
    // Encode the access byte
    target[5] = source.access_byte;
    
    // Encode the flags
    target[6] |= (source.flags << 4);

}

void initGdt(){
    tss.ss0 = 0x10; //kernel data gdt section
    tss.esp0 = 0x0; // esp0 gets value of stack-pointer at syscall
    tss.iopb = sizeof(tss); 

    GDT NullDescriptor = {0, 0, 0, 0};
    GDT KernelCode = {0, 0xFFFFF, 0x9A, 0xC};
    GDT KernelData = {0, 0xFFFFF, 0x92, 0xC}; /*flags- bit 0 reserved, 
    bit 1 enabled long mode. bit 2 is 16 bit protected when 0, 
    32 bit when 1, bit 3 enables granularity- when 0, limits are measured in 1 byte blocks
    , when 1, 4 KiB blocks */
    GDT UserCode = {0, 0xFFFFF, 0xFA, 0xC};
    GDT UserData = {0, 0xFFFFF, 0xF2, 0xC};
    GDT TaskStateSegment= {(uint32_t)&tss, (uint32_t)sizeof(tss)-1, 0x89, 0x0};
    
    encodeGdtEntry(&gdt[0], NullDescriptor);
    encodeGdtEntry(&gdt[8], KernelCode);
    encodeGdtEntry(&gdt[16], KernelData);
    encodeGdtEntry(&gdt[24], UserCode);
    encodeGdtEntry(&gdt[32], UserData);
    encodeGdtEntry(&gdt[40], TaskStateSegment);
    setGDT(gdtsize, gdt);
    reloadSegments();
    reloadTSS();
    terminal_writestring("GDT loaded successfully...\n");
}