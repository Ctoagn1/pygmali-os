#include <stdint.h>

void paging_setup();
void create_page_tables(uint32_t* page_directory);
void set_memory_bitmap();
void reserve_address(uint32_t start, uint32_t end);
uint32_t alloc_page();
uint32_t alloc_raw_page();
void free_raw_page(uint32_t addr);
uint32_t map_page(uint32_t physaddr);
void free_page(uint32_t addr);
void unreserve_address(uint32_t start, uint32_t end);