#include <stdint.h>
#include "kmalloc.h"
#include "string.h"
#include "paging.h"
#include "tty.h"
#define MAX_PAGES 1024*1024
extern int*_kernel_endpoint;
extern void enable_paging();
typedef struct{
    uint64_t base_address;
    uint64_t mem_length;
    uint32_t region_type;
    uint32_t acpi_attributes; //note- some bioses will leave this empty
} Memory_Entry;
uint32_t pagenum=0;
uint8_t page_bitmap[MAX_PAGES/8]={0};
uint8_t virt_page_bitmap[MAX_PAGES/8]={0};
uint8_t table_bitmap[128]={0};
uint32_t fbuffer_pages=(4*1024*1280+4095)/4096;
const uint32_t page_size = 4096;
const Memory_Entry* memory_map_array = (Memory_Entry*)0xc0010004;

void invlpg(unsigned long addr) {
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
void paging_setup(){
    set_memory_bitmap();
    reserve_address(0, 0x400000);
    reserve_address(selected_video_mode->framebuffer, selected_video_mode->framebuffer+selected_video_mode->bpp*selected_video_mode->width+selected_video_mode->height);
    memset(virt_page_bitmap, 0xFF, sizeof(virt_page_bitmap));
    memset(get_page_table_virtual(0)[0], 0, 4096); //unmap original identity mapping
}
uint32_t* get_page_table_virtual(uint32_t dir_index){
   return (uint32_t*)(0xFFC00000+dir_index*page_size);
}

void set_memory_bitmap(){
    const uint32_t memory_map_length = *((uint32_t*)0x10000);
    for(int i=0; i<memory_map_length; i++){
        if(memory_map_array[i].region_type==1 && pagenum<MAX_PAGES){ // 1 signals usable memory
            uint32_t offset = 0;
            uint32_t start = memory_map_array[i].base_address;
            start = (start%page_size==0) ? start : ((start/page_size)+1)*page_size;
            while(start+offset<=memory_map_array[i].base_address+memory_map_array[i].mem_length+page_size){
                page_bitmap[(start+offset)/(8*page_size)] |= (1<<(((start+offset)/page_size)%8)); //1 is free, 0 is used
                pagenum++;
                if(pagenum==MAX_PAGES) break;
                offset+=page_size;
            }
        }
    }
    uint32_t aligned_fbuffer =selected_video_mode->framebuffer-(selected_video_mode->framebuffer%4096);
    uint32_t starting_page = aligned_fbuffer/4096;
    for(int i=0; i<fbuffer_pages; i++){
        page_bitmap[(starting_page+i)/8]&=~(1<<((starting_page+i)%8));
    }
}
void reserve_address(uint32_t start, uint32_t end){
    start/=page_size;
    end=(end/page_size)+1;
    for(uint32_t i=start; i<end; i++){
        page_bitmap[i/8]&=~(1<<(i%8));
    }
}
void unreserve_address(uint32_t start, uint32_t end){
    start/=page_size;
    end=(end/page_size)+1;
    for(uint32_t i=start; i<end; i++){
        page_bitmap[i/8]|=(1<<(i%8));
    }
}
uint32_t alloc_page(uint32_t virt_addr){
    uint32_t virtpage=virt_addr/4096;
    if(virt_page_bitmap[virtpage/8]&(1<<(virtpage%8))){
        virt_page_bitmap[virtpage/8]&=~(1<<(virtpage%8));
        uint32_t phys_addr = alloc_raw_page();
        map_page(virt_addr, phys_addr);
        return 0;
    }
    else{
        return 1;
    }
}
uint32_t alloc_raw_page(){
    for(uint32_t i=0; i<MAX_PAGES; i++){
        if(page_bitmap[i/8]&(1<<(i%8))){
            page_bitmap[i/8]&=~(1<<(i%8));
            return i*page_size;
        }
    }
    return 0;
}
void free_raw_page(uint32_t addr){
    uint32_t i=addr/page_size;
    page_bitmap[i/8] |= (1<<(i%8));
}
uint32_t map_page(uint32_t virtaddr, uint32_t physaddr){
    uint32_t* dir = get_page_table_virtual(1023); // virtual address of page directory
    uint32_t dir_index = (virtaddr >> 22) & 0b1111111111;
    if (!(dir[dir_index] & 0x1)) {
        create_new_table(virtaddr);
    }
    uint32_t* table = get_page_table_virtual((virtaddr>>22)&0b1111111111); //extract 10 table bits
    table = (uint32_t*)((uint32_t)table & 0xfffff000);
    table[(virtaddr>>12)&0b1111111111]=physaddr|0x3;
    invlpg(virtaddr);
    memset((void*)virtaddr, 0, 4096);
    return 0;
}
void free_page(uint32_t addr){
    uint32_t table_entry = (addr>>22);
    if(!((uint32_t)get_page_table_virtual(table_entry)&0x1)) return;
    uint32_t table_offset = ((addr>>12)&1023);
    virt_page_bitmap[(addr/4096)/8] |= 1<<((addr/4096)%8);
    uint32_t* table = get_page_table_virtual(table_entry);
    table = (uint32_t*)((uint32_t)table & 0xfffff000);
    uint32_t physaddr = table[table_offset]&0xFFFFF000;
    table[table_offset] = 0;
    invlpg(addr);
    free_raw_page(physaddr);
}
void create_new_table(uint32_t addr){
    uint32_t* dir = get_page_table_virtual(1023);
    uint32_t tablenum = (addr>>22)&0b1111111111;
    dir[tablenum]=alloc_raw_page()|0x3;
    void* table_va = (void*)(0xFFC00000)+(tablenum<<12);
    invlpg(table_va);
    memset(get_page_table_virtual(tablenum), 0, 4096);
}

