#include <stdint.h>
#include "kmalloc.h"
#include "string.h"
#include "paging.h"
#include "tty.h"
#define MAX_PAGES 65536
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
const uint32_t page_size = 4096;
const Memory_Entry* memory_map_array = (Memory_Entry*)0x10004;

void paging_setup(){
    set_memory_bitmap();
    reserve_address(0, 0x400000);
    uint32_t* page_directory=(uint32_t*)alloc_raw_page();
    memset(page_directory, 0, 4096);
    page_directory[1023]=(uint32_t)page_directory | 3; //recursive mapping
    memset(virt_page_bitmap, 0xFF, sizeof(virt_page_bitmap));
    create_page_tables(page_directory);
    enable_paging(page_directory);
    
}
uint32_t* get_page_table_virtual(uint32_t dir_index){
   return (uint32_t*)(0xFFC00000+dir_index*page_size);
}
void create_page_tables(uint32_t* page_directory){
    uint32_t* first_page_table = (uint32_t*)alloc_raw_page();
    unreserve_address(0, 0x400000);
    memset(first_page_table, 0, 4096);

    for(int i=0; i<1024; i++){
        first_page_table[i]=(i*0x1000)|0x3; //supervisor, write-enabled, present
    }
    page_directory[0]=((uint32_t)first_page_table)|3;

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
uint32_t alloc_page(){
    for(uint32_t i=0; i<MAX_PAGES; i++){
        if(page_bitmap[i/8]&(1<<(i%8))){
            page_bitmap[i/8]&=~(1<<(i%8));
            uint32_t physaddr= i*page_size;
            uint32_t virtaddr=map_page(physaddr);
            return virtaddr;
        }
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
uint32_t map_page(uint32_t physaddr){
    for(uint32_t i=0; i<MAX_PAGES; i++){
        if(virt_page_bitmap[i/8]&(1<<(i%8))){
            virt_page_bitmap[i/8]&=~(1<<(i%8));
            if(!(get_page_table_virtual(1023)[i/1024]&0x1)){ //the directory is also the 1023 entry in itself. recursion is weird
                uint32_t new_table_entry=alloc_raw_page();
                get_page_table_virtual(1023)[i/1024] = new_table_entry|0x3;
                table_bitmap[i/1024]=1;
            }
            uint32_t* table_addr = (uint32_t*)(get_page_table_virtual(1023)[i/1024]&0xFFFFF000);
            table_addr[i%1024]=physaddr|0x3;
            uint32_t virt_addr = ((i/1024)<<22)|((i%1024)<<12); //top 10 bits for directory offset, middle 10 for table offset, 
            memset((uint32_t*)virt_addr, 0, 4096);
            return virt_addr;
        }
    }
    return 0;
}
void free_page(uint32_t addr){
    uint32_t table_entry = (addr>>22);
    if(!(get_page_table_virtual(1023)[table_entry]&0x1)) return;
    uint32_t table_offset = ((addr>>12)&1023);
    virt_page_bitmap[(table_offset+table_entry*1024)/8] |= 1<<(table_offset%8);
    uint32_t* table = (uint32_t*)get_page_table_virtual(1023)[table_entry];
    uint32_t physaddr = table[table_offset]&0xFFFFF000;
    table[table_offset] = 0;
    free_raw_page(physaddr);
}