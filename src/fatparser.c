#include "diskreader.h"
#include "string.h"
#include "kmalloc.h"
#include "fatparser.h"
#include "printf.h"
#define FAT_NAME_LENGTH 11
#define MAX_PATH_DEPTH 64
uint16_t bytes_per_sector;
uint8_t sectors_per_cluster;
uint16_t reserved_sectors;
uint8_t num_of_fats;
uint32_t sectors_per_fat;
uint32_t root_cluster;
uint64_t first_data_sector;
char* working_dir="/";



uint32_t sector_of_cluster(int clusternum){
    return((clusternum-2)*sectors_per_cluster)+first_data_sector; //clusters 0 and 1 are reserved, so -2
}
void read_boot_record(){
    uint8_t boot_record[512];
    read_sector(partition_start, boot_record);
    bytes_per_sector=merge_bytes(&boot_record[11], 2);
    sectors_per_cluster=boot_record[13];
    reserved_sectors=merge_bytes(&boot_record[14], 2);
    num_of_fats=boot_record[16];
    sectors_per_fat=merge_bytes(&boot_record[36], 4);
    root_cluster=merge_bytes(&boot_record[44], 4);
    first_data_sector=partition_start+reserved_sectors+num_of_fats*sectors_per_fat;
}
int get_from_fat(int cluster_num){
    unsigned char* fat_table = kmalloc(512);
    if(!fat_table) return -1;
    int fat_offset=cluster_num*4;
    int fat_sector = partition_start+reserved_sectors +(fat_offset/512);
    int entry_offset =fat_offset%512;
    read_sector(fat_sector, fat_table);
    unsigned int dest_cluster = merge_bytes((uint8_t*)&fat_table[entry_offset], 4);
    kfree(fat_table);
    return dest_cluster;
}
DirectoryListing directory_parse(int cluster_num){ //caller must free result.entries
    DirectoryListing result = {0};
    uint8_t* cluster = kmalloc(sectors_per_cluster*512);
    if(!cluster) return result;
    int sector_num=sector_of_cluster(cluster_num);
    int entries_per_cluster = sectors_per_cluster*512/sizeof(Cluster_Entry);
    while(cluster_num<0x0FFFFFF8){ //signals end of dir/file
        sector_num=sector_of_cluster(cluster_num);
        for(int i=0; i<sectors_per_cluster; i++){
            read_sector(sector_num+i, &cluster[512*i]);
        }
        result.entries = krealloc(result.entries, (result.count + entries_per_cluster)*sizeof(Cluster_Entry));
        memcpy(&result.entries[result.count], cluster, entries_per_cluster*sizeof(Cluster_Entry));
        cluster_num=get_from_fat(cluster_num);
        result.count += entries_per_cluster;
    }
    kfree(cluster);
    return result;
}
char* filename_to_plaintext(char filename[11]){ //caller must free file_plaintext
    char* file_plaintext = kmalloc(FAT_NAME_LENGTH+2); //+2 for null terminator and period
    memset(file_plaintext, 0, FAT_NAME_LENGTH+2);
    int name_len=0;
    for(int i=0; i<8; i++){
        if(filename[i]!=' '){
            name_len++;
            file_plaintext[name_len-1]=filename[i];
        }
    }
    if(filename[8]!=' ' || filename[9]!=' ' || filename[10]!=' '){
        name_len++;
        file_plaintext[name_len-1]='.';
    }
    for(int i=8; i<11; i++){
        if(filename[i]!=' '){
            name_len++;
            file_plaintext[name_len-1]=filename[i];
        }
    }
    return file_plaintext;
}
char* plaintext_to_filename(char* filename){ //caller must free file_fatname
    to_uppercase(filename);
    if(strlen(filename)>13) return NULL;
    if(strlen(filename)==0) return NULL;
    char* file_fatname = kmalloc(FAT_NAME_LENGTH);
    memset(file_fatname, ' ', FAT_NAME_LENGTH);
    int extension_start=0;
    _Bool period_exists=0;
    _Bool valid_fat=1;
    for(int i=0; i<strlen(filename); i++){
        if(filename[i]=='.' && period_exists) return NULL;
        if(filename[i]=='.'){
            period_exists=1;
            extension_start=i+1;
        }
        if(!((filename[i]>= 'A' && filename[i]<='Z') || (filename[i]>='0' && filename[i]<='9') || filename[i]=='_' || filename[i]=='.')) valid_fat=0;
    }
    if (valid_fat==0) return NULL;
    if(extension_start>9) return NULL;
    if(period_exists && strlen(filename)-extension_start>3) return NULL;
    for(int i=0; i<8; i++){
        if(filename[i]=='\0') return file_fatname;
        if(filename[i]=='.'){
            break;
        }
        file_fatname[i]=filename[i];
    } 
    for(int i=0; i<3; i++){
        if(filename[extension_start+i]!='\0') file_fatname[8+i]=filename[extension_start+i];
    }
    return file_fatname;
}
char* names_from_directory(DirectoryListing list){ //caller must free list
    int file_num=0;
    char* name_list = NULL;
    int total_length=0;
    int start_pos=0;
    for(int i=0; i<list.count; i++){
        if(list.entries[i].name[0]==0) break;
        if(list.entries[i].name[0]==0xE5) continue; //skip over deleted files
        if((list.entries[i].attr & 0b00001111) == 0b00001111) continue; //skip over long file names
        char* plain_filename = filename_to_plaintext(list.entries[i].name);
        total_length+=(strlen(plain_filename)+1);
        name_list = krealloc(name_list, total_length);
        memcpy(&name_list[start_pos],plain_filename, strlen(plain_filename));
        name_list[total_length-1]=' ';
        start_pos=total_length;
        kfree(plain_filename);
    }
    if(name_list) name_list[total_length-1]='\0';
    return name_list;
}
int file_path_destination(char* input_dir){
    if(input_dir[0] !='/') return -1; //absolute paths only
    int current_cluster=root_cluster;
    int length = strlen(input_dir);
    int current_location=1;
    int cluster_stack[MAX_PATH_DEPTH];
    int stack_top=0;
    int location_in_dirname=0;
    char nextdir[13]={0};
    while(current_location<length){
        location_in_dirname=0;
        while(input_dir[current_location]!='/' && current_location<length){
            nextdir[location_in_dirname]=input_dir[current_location];
            location_in_dirname++;
            current_location++;
            if(location_in_dirname >= 12) return -1; 
        }
        if(input_dir[current_location] == '/') current_location++;
        if(nextdir[0]=='\0'){
            memset(nextdir, 0, sizeof(nextdir));
            continue;
        }
        nextdir[location_in_dirname] = '\0';
        if(strcmp(".", nextdir)==0){
            memset(nextdir, 0, sizeof(nextdir));
            continue;
        }
        if(strcmp("..", nextdir)==0){
            if (stack_top==0) return -1;
            current_cluster = cluster_stack[--stack_top];
            memset(nextdir, 0, sizeof(nextdir));
            continue;
        }
        if(stack_top>= MAX_PATH_DEPTH) return -1;
        cluster_stack[stack_top++]=current_cluster;
        DirectoryListing current_dir_results= directory_parse(current_cluster);
        char* file_entry = plaintext_to_filename(nextdir);
        memset(nextdir, 0, sizeof(nextdir));
        current_cluster = -1;
        for(int i=0; i<current_dir_results.count; i++){
            if(strncmp(current_dir_results.entries[i].name, file_entry, 11)==0 && ((current_dir_results.entries[i].attr & 0x10) != 0)){
                current_cluster=(uint32_t)current_dir_results.entries[i].low_cluster_number+((uint32_t)current_dir_results.entries[i].high_cluster_number<<16);
            }
        }
        kfree(file_entry);
        kfree(current_dir_results.entries);
        if(current_cluster==-1) return -1;
    }
    return current_cluster;
}
char* append_path(char* filepath){ //caller must free
    if(filepath[0]=='/'){
        return strdup(filepath);
    }
    int wd_len=strlen(working_dir);
    int fp_len=strlen(filepath);
    _Bool add_slash=!(working_dir[wd_len-1]=='/');
    char* new_filepath = kmalloc(fp_len+wd_len+add_slash+1);//1 for null terminator
    memcpy(new_filepath, working_dir, wd_len);
    if(add_slash)new_filepath[wd_len]='/';
    memcpy(new_filepath+wd_len+add_slash, filepath, fp_len);
    new_filepath[wd_len+add_slash+fp_len]='\0';
    return new_filepath;
}
void normalize_path(char *path){
    to_uppercase(path);
    char *src = path;
    char *dst = path;
    char *segments[64];
    int depth = 0;

    while (*src) {
        while (*src == '/') src++;

        if (*src == '\0') break;

        char *seg_start = src;
        while (*src && *src != '/') src++;

        int len = src - seg_start;

        if (len == 1 && seg_start[0] == '.') {
            continue;
        } else if (len == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
            if (depth > 0) depth--;
        } else {
            segments[depth++] = seg_start;
        }
    }

    if (depth == 0) {
        *dst++ = '/';
    } else {
        for (int i = 0; i < depth; i++) {
            *dst++ = '/';
            char *seg = segments[i];
            while (*seg && *seg != '/') *dst++ = *seg++;
        }
    }
    *dst = '\0';
}
                               