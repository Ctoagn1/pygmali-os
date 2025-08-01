#include <stdint.h>
#include <stddef.h>
uint16_t* disk_info();
void read_sector(uint32_t sector, uint8_t data_buffer[512]);
void write_sector(uint32_t sector, uint8_t* sector_data);
extern uint32_t partition_start;
extern uint32_t partition_end;
void scan_mbr();
