#include "paging.h"
#include <stdint.h>
#include "kmalloc.h"
#include "string.h"
#include "tty.h"
#include "process.h"
#include <stdbool.h>

#define MAX_PAGES 1024*1024
#define KERNEL_BITMAP_SIZE 0x8000
#define OFFSET 0xC0000000
extern int*_kernel_endpoint;
extern void enable_paging();
typedef struct{
    uint64_t base_address;
    uint64_t mem_length;
    uint32_t region_type;
    uint32_t acpi_attributes; //note- some bioses will leave this empty
} Memory_Entry;

uint32_t pagenum=0;
uint32_t kernel_pd;
uint8_t page_bitmap[MAX_PAGES/8]={0};
uint8_t kernel_virt_page_bitmap[KERNEL_BITMAP_SIZE]={0};
uint32_t fbuffer_pages=(4*1024*1280+4095)/4096;
const uint32_t page_size = 4096;
const Memory_Entry* memory_map_array = (Memory_Entry*)0xc0010004;
extern ProcessNode* current_process;


void invlpg(unsigned long addr) { 
    /*as translating a virtual address takes multiple memory accesses,
    translated addresses are kept in a lookup buffer. pages in this buffer need to be
    invalidated when switched */
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
void page_fault_handler(uint32_t pageaddr, uint32_t errcode){
    bool is_user = errcode & 0x4;
    bool is_write = errcode & 0x2;
    bool present = errcode & 0x1;
    if(present && !is_user){ //protection fault
        panic("PAGE PROTECTION FAULT");
    }
    else if((present||pageaddr>=0xC0000000) && is_user){
        kill_process(current_process);
    }
    else{
        alloc_page(pageaddr);
    }
}
void paging_setup(){
    set_memory_bitmap();
    reserve_address(0, 0x400000);
    reserve_address(selected_video_mode->framebuffer, selected_video_mode->framebuffer+selected_video_mode->bpp*selected_video_mode->width+selected_video_mode->height);
    memset(kernel_virt_page_bitmap, 0xFF, sizeof(kernel_virt_page_bitmap));
    memset(get_page_table_virtual(0)[0], 0, 4096); //unmap original identity mapping
    kernel_pd = get_page_table_virtual(1023);
}
uint32_t* get_page_table_virtual(uint32_t dir_index){
   return (uint32_t*)(0xFFC00000+dir_index*page_size);
}
uint32_t phys_from_virt(void* virt) {
    uint32_t v = (uint32_t)virt;
    uint32_t dir_index = v >> 22;
    uint32_t table_index = (v >> 12) & 0x3FF;
    uint32_t offset = v & 0xFFF;
    uint32_t entry = get_page_table_virtual(dir_index)[table_index];
    if (!(entry & 0x1)) {
        return 0;
    }
    uint32_t phys_base = entry & 0xFFFFF000;
    return phys_base | offset;
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
    uint32_t normalized_virtpage = (virt_addr-OFFSET)/4096;
    if(!current_process){
        if(kernel_virt_page_bitmap[normalized_virtpage/8]&(1<<(normalized_virtpage%8))){
            kernel_virt_page_bitmap[normalized_virtpage/8]&=~(1<<(normalized_virtpage%8));
            uint32_t phys_addr = alloc_raw_page();
            map_page(virt_addr, phys_addr);
            return 0;
        }
        else{
            return 1;
        }
    }
    else{
        uint8_t* bitmap = current_process->p.page_bitmap;
        if(bitmap[virtpage/8]&(1<<(virtpage%8))){
            bitmap[virtpage/8]&=~(1<<(virtpage%8));
            uint32_t phys_addr = alloc_raw_page();
            map_page(virt_addr, phys_addr);
            return 0;
        }
        else return 1;
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

uint32_t create_process_address_space(){
    uint32_t free_address;
    for(int i=0; i<KERNEL_BITMAP_SIZE; i++){
        if((kernel_virt_page_bitmap[i/8]&1<<(i%8))!=0){
            free_address = i*4096+OFFSET;
            break;
        }
    }
    uint32_t* pd = alloc_page(free_address);
    uint32_t phys_pd = phys_from_virt(pd);
    memcpy(&pd[768], ((uint32_t*)kernel_pd)[768], 255*sizeof(uint32_t));
    pd[1023]=phys_pd|0x3;
    memset(&pd[0], 0, 768*sizeof(uint32_t)); 
    free_page(free_address);
    page_bitmap[phys_pd/8]&=~(1<<phys_pd%8);
    return phys_pd;
}

uint32_t map_page(uint32_t virtaddr, uint32_t physaddr){
    uint32_t* dir = get_page_table_virtual(1023); // virtual address of page directory
    uint32_t dir_index = (virtaddr >> 22) & 0b1111111111;
    int user = 0;

    if (!(dir[dir_index] & 0x1)) {
        uint32_t phys_table = create_new_table(virtaddr);
        if(current_process!=NULL){
            user=1;
            PageNode* newpage = kmalloc(sizeof(PageNode));
            newpage->physical_page = phys_table;
            newpage->next = current_process->p.tablelist;
            current_process->p.tablelist = newpage;
        }
    }

    if(current_process!=NULL){
        user = 1;
        PageNode* newpage = kmalloc(sizeof(PageNode));
        newpage->virtual_page = virtaddr;
        newpage->physical_page = physaddr;
        newpage->next = current_process->p.pagelist;
        current_process->p.pagelist = newpage;
    }

    uint32_t* table = get_page_table_virtual((virtaddr>>22)&0b1111111111); //extract 10 table bits
    table = (uint32_t*)((uint32_t)table & 0xfffff000);
    table[(virtaddr>>12)&0b1111111111]=physaddr|0x3|(user<<2);
    invlpg(virtaddr);
    memset((void*)virtaddr, 0, 4096);
    return 0;
}
void free_page(uint32_t addr){
    uint32_t table_entry = (addr>>22);
    if(!((uint32_t)get_page_table_virtual(table_entry)&0x1)) return;
    uint32_t table_offset = ((addr>>12)&1023);
    if(!current_process){
        kernel_virt_page_bitmap[((addr-OFFSET)/4096)/8] |= 1<<(((addr-OFFSET)/4096)%8);
    }
    else{
        uint8_t* bitmap = current_process->p.page_bitmap;
        bitmap[addr/4096/8] |= 1<<(addr/4096%8);
    }
    uint32_t* table = get_page_table_virtual(table_entry);
    table = (uint32_t*)((uint32_t)table & 0xfffff000);
    uint32_t physaddr = table[table_offset]&0xFFFFF000;
    table[table_offset] = 0;
    invlpg(addr);
    free_raw_page(physaddr);
}
uint32_t create_new_table(uint32_t addr){
    uint32_t* dir = get_page_table_virtual(1023);
    uint32_t tablenum = (addr>>22)&0b1111111111;
    uint32_t physaddr = alloc_raw_page();
    dir[tablenum]=physaddr|0x3;
    void* table_va = (void*)(0xFFC00000)+(tablenum<<12);
    invlpg(table_va);
    memset(get_page_table_virtual(tablenum), 0, 4096);
    return physaddr;
}

