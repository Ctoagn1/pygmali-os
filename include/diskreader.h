uint16_t* disk_info();
void read_sector(uint32_t sector, uint8_t data_buffer[512]);
void write_sector(uint32_t sector, uint8_t* sector_data);
void scan_mbr();
void make_mbr();