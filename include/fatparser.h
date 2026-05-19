#ifndef FATPARSE
#define FATPARSE
#include "diskreader.h"
#include "string.h"
#include "kmalloc.h"
#include "fd.h"
#define FAT_NAME_LENGTH 11
extern uint16_t bytes_per_sector;
extern uint8_t sectors_per_cluster;
extern uint16_t reserved_sectors;
extern uint8_t num_of_fats;
extern uint32_t sectors_per_fat;
extern uint32_t root_cluster;
extern uint64_t first_data_sector;
extern char* working_dir;
typedef struct{
    unsigned char name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t creation_time;
    uint16_t time_of_creation;
    uint16_t date_of_creation;
    uint16_t date_last_accessed;
    uint16_t high_cluster_number;
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint16_t low_cluster_number;
    uint32_t size_of_file;
}__attribute__((packed)) Cluster_Entry;

typedef struct{
    Cluster_Entry* entries;
    int count;
}__attribute__((packed)) DirectoryListing;

typedef struct {
    int lba;
    int byte_offset;
    int cluster;
}__attribute__((packed)) File_Location;

typedef enum{
    FIND_EXISTS,
    FIND_NEW
} LookupMode;

typedef enum{
    FILE,
    DIRECTORY
} FileType;

uint32_t sector_of_cluster(int clusternum);
extern uint8_t num_of_fats;
int get_from_fat(int cluster_num);
int delete_file(char* filename);
void read_boot_record();
DirectoryListing directory_parse(int cluster_num);
char* filename_to_plaintext(unsigned char *filename);
unsigned char* plaintext_to_filename(char* filename);
File_Location get_file_location(char* filename, LookupMode mode);
char* names_from_directory(DirectoryListing list);
int file_path_destination(char* input_dir);
int check_attributes(char* filename);
char* append_path(char* filepath);
void normalize_path(char *path); 
int fat32_read(File* f, void* buf, int n);
int fat32_write(File* f, const void* buf, int n);
int create_file(char* filename, FileType type);
int extend_file(int cluster);
uint32_t get_empty_cluster();
int file_size_from_name(char* filename);
int assign_filedata(File* f,  char* filename);
#endif