#include "paging.h"
#include <stdint.h>
#include "kmalloc.h"
#include "string.h"
#include "console.h"
#include "multiboot.h"
#include "idt.h"
#include "process.h"
#include <stdbool.h>

#define MAX_PAGES 1024*1024
#define KERNEL_BITMAP_SIZE 0x8000
#define OFFSET 0xC0000000
#define USER_PD_AREA 0xD0000000
extern int*_kernel_endpoint;
extern void enable_paging();


uint32_t pagenum=0;
uint32_t* kernel_pd;
uint8_t page_bitmap[MAX_PAGES/8]={0};
uint8_t kernel_virt_page_bitmap[KERNEL_BITMAP_SIZE]={0};
uint32_t fbuffer_pages=(4*1024*1280+4095)/4096;
const uint32_t page_size = 4096;
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
    (void)is_write;
    bool present = errcode & 0x1;
    if(present && !is_user){ //protection fault
        panic("PAGE PROTECTION FAULT");
    }
    else if((present||pageaddr>=0xC0000000) && is_user){
        kill_process(current_process->p.pid);
    }
    else{
        alloc_page(pageaddr);
    }
}
void paging_setup(struct multiboot_tag_mmap* mmap){
    set_memory_bitmap(mmap);
    reserve_address(0, 0x400000);
    reserve_address(selected_video_mode->common.framebuffer_addr, selected_video_mode->common.framebuffer_addr+selected_video_mode->common.framebuffer_bpp*selected_video_mode->common.framebuffer_width+selected_video_mode->common.framebuffer_height);
    memset(&kernel_virt_page_bitmap[1024/8], 0xFF, sizeof(kernel_virt_page_bitmap)-4*1024/8); //first table already mapped, last 3 are for screen and recursive
    memset(&get_page_table_virtual(0)[0], 0, 4096); //unmap original identity mapping
    kernel_pd = get_page_table_virtual(1023);
    uint32_t phys_pd = phys_from_virt(kernel_pd);
    asm volatile("mov %0, %%cr3" :: "r"(phys_pd)); //flush tlb
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
void set_memory_bitmap(struct multiboot_tag_mmap* mmap){
    uint8_t *ptr = (uint8_t*)mmap->entries;
    uint32_t total_entries = (mmap->size - sizeof(*mmap)) / mmap->entry_size;
    for(unsigned int i=0; i<total_entries; i++){
        struct multiboot_mmap_entry *e = (struct multiboot_mmap_entry*)ptr;
        if(e->type==1 && pagenum<MAX_PAGES){ // 1 signals usable memory
            uint64_t start = e->addr;
            uint64_t end = e->addr + e->len;
            start = (start+page_size-1)& ~(page_size-1);
            for(uint64_t addr = start; addr+page_size <= end && pagenum < MAX_PAGES; addr += page_size){
                uint32_t page_index = addr>>12; //same as addr/page_size but no 64 bit division
                page_bitmap[page_index/8]|=(1<<(page_index%8));
                pagenum++;
            }
        }
        ptr+=mmap->entry_size;
    }
    uint32_t aligned_fbuffer =selected_video_mode->common.framebuffer_addr-(selected_video_mode->common.framebuffer_addr&0xFFF);
    uint32_t starting_page = aligned_fbuffer>>12;
    for(unsigned int i=0; i<fbuffer_pages; i++){
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
    if(virt_addr==0){
        panic("NULL POINTER DEREFERENCE");
    }
    uint32_t virtpage=virt_addr>>12;
    uint32_t normalized_virtpage = (virt_addr-OFFSET)>>12;
    if(virt_addr>=0xC0000000){
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

Page create_process_address_space(){
    uint32_t free_address;
    for(int i=USER_PD_AREA/4096/8; i<KERNEL_BITMAP_SIZE*8; i++){
        if((kernel_virt_page_bitmap[i/8]&(1<<(i%8)))!=0){
            free_address = i*4096+OFFSET;
            break;
        }
    }
    alloc_page(free_address);
    uint32_t* pd = (uint32_t*)free_address;
    memcpy(&pd[768], &((uint32_t*)kernel_pd)[768], 255*sizeof(uint32_t));
    uint32_t phys_pd = phys_from_virt(pd);
    pd[1023]=phys_pd|0x3;
    memset(&pd[0], 0, 768*sizeof(uint32_t)); 
    Page user_pd = {phys_pd, (uint32_t)pd};
    return user_pd;
}

uint32_t map_page(uint32_t virtaddr, uint32_t physaddr){
    bool is_kernel = virtaddr>=0xC0000000;
    uint32_t* dir = get_page_table_virtual(1023); // virtual address of page directory
    uint32_t dir_index = (virtaddr >> 22) & 0b1111111111;
    int user = 0;

    if (!(dir[dir_index] & 0x1)) {
        uint32_t phys_table = create_new_table(virtaddr);
        if(!is_kernel){
            user=1;
            PageNode* newpage = kmalloc(sizeof(PageNode));
            newpage->physical_page = phys_table;
            newpage->next = current_process->p.tablelist;
            current_process->p.tablelist = newpage;
        }
    }

    if(!is_kernel){
        user = 1;
        PageNode* newpage = kmalloc(sizeof(PageNode));
        newpage->virtual_page = virtaddr;
        newpage->physical_page = physaddr;
        newpage->next = current_process->p.pagelist;
        current_process->p.pagelist = newpage;
    }

    uint32_t* table = get_page_table_virtual((virtaddr>>22)&0b1111111111); //extract 10 table bits
    table = (uint32_t*)((uint32_t)table & 0xfffff000);
    table[(virtaddr>>12)&0b1111111111]=physaddr|0x3|(user<<2)|(is_kernel<<7);
    invlpg(virtaddr);
    memset((void*)virtaddr, 0, 4096);
    return 0;
}
void free_page(uint32_t addr){
    uint32_t table_entry = (addr>>22);
    if(!((uint32_t)get_page_table_virtual(table_entry)&0x1)) return;
    uint32_t table_offset = ((addr>>12)&1023);
    if(addr>=0xC0000000){
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
    bool is_kernel = addr>=0xc0000000;
    uint32_t* dir = get_page_table_virtual(1023);
    uint32_t tablenum = (addr>>22)&0b1111111111;
    uint32_t physaddr = alloc_raw_page();
    physaddr|=0x3;
    if(is_kernel)physaddr|=(1<<7); //mark global bit, will never be flushed by cr3 switch
    else physaddr |= (1<<2); //user bit 
    dir[tablenum]=physaddr;
    //void* table_va = (void*)(0xFFC00000)+(tablenum<<12);
    //invlpg(table_va);
    memset(get_page_table_virtual(tablenum), 0, 4096);
    return physaddr;
}

