
#include "diskreader.h"
#include "string.h"
#include "kmalloc.h"
#define FAT_NAME_LENGTH 11
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
char* file_contents(char* filename);
int create_file(char* filename, FileType type);