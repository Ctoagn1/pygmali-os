#define PRIMARY_BUS 0x1F0 //0x1F0-0x1F7
#define PRIMARY_BUS_CONTROL 0x3F6 //and 0x3F7
#define SECONDARY_BUS 0x170 //0x170-0x177
#define SECONDARY_BUS_CONTROL 0x376 //and 0x377
#define DATA_REGISTER_OFFSET 0 //16 bits!
#define ERROR_REGISTER_OFFSET 1
#define SECTOR_COUNT_REGISTER_OFFSET 2
#define LBA_LOW_REGISTER_OFFSET 3
#define LBA_MED_REGISTER_OFFSET 4
#define LBA_HIGH_REGISTER_OFFSET 5
#define DRIVE_REGISTER_OFFSET 6
#define STATUS_REGISTER_OFFSET 7
#define COMMAND_REGISTER_OFFSET 7 //depends if reading or writing
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16
#define GPT_PARTITION_ENTRY_OFFSET 0x48
#define GPT_PARTITION_NUM_OFFSET 0x50
#define GPT_IDENTIFIER_OFFSET 0x38
#define GPT_PARTITION_ENTRY_LEN 128
#define GPT_START_LBA 0x20
#define GPT_END_LBA 0x28
//uses 28 bit, bytes 3,4,5, lower nibble of 6
#include "io.h"
#include "tty.h"
#include "string.h"
#include "kmalloc.h"
#include "printf.h"
uint16_t diskinfo[256]={0};//identify returns 512 bytes (or 256 words)
uint16_t sectorinfo[256]={0};//identify returns 512 bytes (or 256 words)
uint32_t partition_start=0;
uint32_t partition_end=0;
uint32_t bus_select = PRIMARY_BUS;
int sector_space=1;
_Bool startup_read=1;
_Bool is_slave_drive=0;
char gpt_identifier[37]={0};//gpt identifier is 72 bytes of 2-byte characters, +1 for null terminator


void mega_wait(){ //disk can take 400ns, ~15 io_waits
    for(int i=0; i<15; i++){
        io_wait();
    }
}
uint16_t* disk_info(){
    int timer=10000;
    outb(bus_select+DRIVE_REGISTER_OFFSET, 0b11100000 | (is_slave_drive<<4)); //bits 7 and 5 are always set, 6 is set for LBA (0 is cylinder-head-sector, obsolete), 4 is master if 0, slave if 1, 3-0 are top bits of lba address, 0000 since it's the first sector
    outb(bus_select+LBA_LOW_REGISTER_OFFSET, 0); //0 out lba registers
    outb(bus_select+LBA_MED_REGISTER_OFFSET, 0);
    outb(bus_select+LBA_HIGH_REGISTER_OFFSET, 0);
    mega_wait();
    while((inb(bus_select+STATUS_REGISTER_OFFSET)&0b10000000)!=0 && --timer) io_wait(); //last bit indicates drive is busy
    if(!timer) return NULL;
    outb(bus_select+COMMAND_REGISTER_OFFSET, 0xEC); //identify command
    while((inb(bus_select+STATUS_REGISTER_OFFSET)&0b10000000)!=0 && --timer) io_wait(); //poll bsy (bit 7) before and after command
    while (!(inb(bus_select + STATUS_REGISTER_OFFSET) & 0b00001000) && --timer) io_wait(); //bit 3 indicates ready to read/write
    if(!timer) return NULL;
    for(int i=0; i<256; i++){
        diskinfo[i]=inw(bus_select+DATA_REGISTER_OFFSET);
    }
    sector_space=(((uint32_t)diskinfo[61])<<16)+(uint32_t)(diskinfo[60]);
    return diskinfo;
}
int read_sector(uint32_t sector, uint8_t data_buffer[512]){
    if(!startup_read &&(sector<partition_start || sector>partition_end)){
        return -1;
    }
    int timer=100000;
    uint16_t temp_array[256];
    outb(bus_select+DRIVE_REGISTER_OFFSET, 0b11100000 | ((sector>>24)&0b00001111) | (is_slave_drive<<4) ); 
    outb(bus_select+LBA_LOW_REGISTER_OFFSET, sector & 0b11111111); 
    outb(bus_select+LBA_MED_REGISTER_OFFSET, (sector>>8) & 0b11111111);
    outb(bus_select+LBA_HIGH_REGISTER_OFFSET, (sector>>16) & 0b11111111);
    outb(bus_select+SECTOR_COUNT_REGISTER_OFFSET, 1); //read from 1 register
    mega_wait();
    while((inb(bus_select+STATUS_REGISTER_OFFSET)&0b10000000)!=0 && --timer) io_wait(); //last bit indicates drive is busy
    if(!timer) return -1;
    outb(bus_select+COMMAND_REGISTER_OFFSET, 0x20); //read command
    while (!(inb(bus_select + STATUS_REGISTER_OFFSET) & 0b00001000) && --timer) io_wait(); 
    while((inb(bus_select+STATUS_REGISTER_OFFSET)&0b10000000)!=0 && --timer) io_wait();
    if(!timer) return -1;
    for(int i=0; i<256; i++){
        temp_array[i]=inw(bus_select+DATA_REGISTER_OFFSET);
    }
    for(int i=0; i<256; i++){
        data_buffer[i*2]=temp_array[i];
        data_buffer[i*2+1]=temp_array[i]>>8;
    }
    return 0;
}
int write_sector(uint32_t sector, uint8_t* sector_data){
     if(sector<partition_start || sector>partition_end|| sector==0){
        return -1;
    }
    int timer=1000000;
    uint16_t *temp_array=kmalloc(512);
    if(!temp_array) return -1;
    for(int i=0; i<256; i++){
        temp_array[i]=sector_data[2*i]+(((uint16_t)sector_data[2*i+1])<<8);
    }
    outb(bus_select+DRIVE_REGISTER_OFFSET, 0b11100000 | ((sector>>24)&0b00001111) | (is_slave_drive<<4) ); 
    outb(bus_select+LBA_LOW_REGISTER_OFFSET, sector & 0b11111111); 
    outb(bus_select+LBA_MED_REGISTER_OFFSET, (sector>>8) & 0b11111111);
    outb(bus_select+LBA_HIGH_REGISTER_OFFSET, (sector>>16) & 0b11111111);
    outb(bus_select+SECTOR_COUNT_REGISTER_OFFSET, 1); //write to 1 register
    mega_wait();
    while((inb(bus_select+STATUS_REGISTER_OFFSET)&0b10000000)!=0 && --timer) io_wait(); //last bit indicates drive is busy
    if(!timer){
        kfree(temp_array);
        return -1;
    }
    outb(bus_select+COMMAND_REGISTER_OFFSET, 0x30); //write command
    mega_wait();
    while((inb(bus_select+STATUS_REGISTER_OFFSET)&0b10000000)!=0 && --timer) io_wait();
    while (!(inb(bus_select + STATUS_REGISTER_OFFSET) & 0b00001000) && --timer) io_wait(); 
    if(!timer){
        kfree(temp_array);
        return -1;
    }
    for(int i=0; i<256; i++){
        outw(bus_select+DATA_REGISTER_OFFSET, temp_array[i]);
    }
    outb(bus_select+COMMAND_REGISTER_OFFSET, 0xE7); //flush write cache
    while((inb(bus_select+STATUS_REGISTER_OFFSET)&0x80) && --timer) io_wait();
    if(!timer){
        kfree(temp_array);
        return -1;
    }
    kfree(temp_array);
    return 0;
}
void scan_gpt(){
    uint32_t lba_of_partition_entries;
    uint32_t num_of_partitions;
    uint8_t gpt[512];
    read_sector(1, gpt);
    if(gpt[0]!=0x45 || gpt[1]!=0x46 || gpt[2]!=0x49 || gpt[3]!=0x20 || gpt[4]!=0x50 || gpt[5]!=0x41 || gpt[6]!=0x52 || gpt[7]!=0x54){ //8 byte GPT signature
        return;
    }
    lba_of_partition_entries=merge_bytes(&gpt[GPT_PARTITION_ENTRY_OFFSET], 8);
    num_of_partitions=merge_bytes(&gpt[GPT_PARTITION_NUM_OFFSET], 4);
    int iteration = 0;
    for(int i=0; i<num_of_partitions; i++){
        read_sector(lba_of_partition_entries+(i/4), gpt);
        int len=0;
        for(int j=0; j<sizeof(gpt_identifier); j++){
            gpt_identifier[j]=gpt[iteration*GPT_PARTITION_ENTRY_LEN+GPT_IDENTIFIER_OFFSET+j*2]; //uses UTF-16, so must check every 2nd character for ASCII
            if(gpt[iteration*GPT_PARTITION_ENTRY_LEN+GPT_IDENTIFIER_OFFSET+j*2]==0 && gpt[iteration*GPT_PARTITION_ENTRY_LEN+GPT_IDENTIFIER_OFFSET+j*2+1]==0)break;
            len++;
        }
        gpt_identifier[len]='\0';
        if(strcmp(gpt_identifier, "PYGMALI_OS")==0){
            partition_start=merge_bytes(&gpt[GPT_START_LBA+iteration*GPT_PARTITION_ENTRY_LEN], 8);
            partition_end=merge_bytes(&gpt[GPT_END_LBA+iteration*GPT_PARTITION_ENTRY_LEN], 8);
            startup_read=0;
            return;
        }
        iteration++;
        iteration %= 4;
    }
    startup_read=0;
}
void scan_mbr(){
#ifdef RAW_DISK_OVERRIDE
        partition_start=0;
        partition_end=UINT32_MAX;
        return;
#endif
    uint8_t mbr[512];
    read_sector(0, mbr);
    if(mbr[PARTITION_TABLE_OFFSET+4]==0xEE){ //represents protective GPT mbr
        scan_gpt();
        printf("PARTITION: %s LBA RANGE: %d, %d\n", gpt_identifier, partition_start, partition_end);
        return;
    }
    return; //TODO- update this logic to read fs data instead of just checking if bootable, for now don't use it
    for(int i=0; i<4; i++){
        if(mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET]==0b10000000 && mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+4]==0x0C){ //is partition bootable and is fat32
            partition_start=(uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+8]+((uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+9]<<8)+((uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+10]<<16)+((uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+11]<<24);
            partition_end=(uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+12]+((uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+13]<<8)+((uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+14]<<16)+((uint32_t)mbr[i*PARTITION_ENTRY_SIZE+PARTITION_TABLE_OFFSET+15]<<24)+partition_start-1;
        }
    }
    startup_read=0;
}
void make_mbr(){ //keeping for reference, not going to use this
    uint8_t mbr[512] = {0};
    mbr[PARTITION_TABLE_OFFSET]=0b00000000; //last bit on if bootable
    mbr[PARTITION_TABLE_OFFSET+4]=0x0C; //partition type, FAT32 with LBA
    mbr[PARTITION_TABLE_OFFSET+8]=1; //begins right after mbr
    mbr[PARTITION_TABLE_OFFSET+13]=0b00000100; //1024 sectors
    mbr[510]=0x55;
    mbr[511]=0xAA;//valid mbr signature
    if(write_sector(0, mbr)==-1) terminal_writestring("Failed to write to disk");
}



