#include <stdint.h>
void paging_setup();
void create_page_tables(uint32_t* page_directory);
void set_memory_bitmap();
void reserve_address(uint32_t start, uint32_t end);
uint32_t alloc_page();
uint32_t alloc_raw_page();
void free_raw_page(uint32_t addr);
uint32_t map_page(uint32_t virtaddr, uint32_t physaddr);
void free_page(uint32_t addr);
void unreserve_address(uint32_t start, uint32_t end);
uint32_t* get_page_table_virtual(uint32_t dir_index);
void page_fault_handler(uint32_t page, uint32_t errcode);
uint32_t create_new_table(uint32_t addr);
uint32_t create_process_address_space();
uint32_t phys_from_virt(void* virt);