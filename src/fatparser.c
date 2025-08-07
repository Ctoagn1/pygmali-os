#include "diskreader.h"
#include "string.h"
#include "kmalloc.h"
#include "fatparser.h"
#include "printf.h"
#include "pit.h"
#define FAT_NAME_LENGTH 11
#define MAX_PATH_DEPTH 64
uint16_t bytes_per_sector;
uint8_t sectors_per_cluster;
uint16_t reserved_sectors;
uint8_t num_of_fats;
uint32_t sectors_per_fat;
uint32_t root_cluster;
uint64_t first_data_sector;
char* working_dir;

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
    working_dir=kmalloc(1);
    working_dir[0]='/';
}
int get_from_fat(int cluster_num){
    if(cluster_num<2) return -1;
    unsigned char* fat_table = kmalloc(512);
    if(!fat_table) return -1;
    int fat_offset=cluster_num*4;
    int fat_sector = partition_start+reserved_sectors +(fat_offset/512);
    if(fat_sector>=partition_start+reserved_sectors+sectors_per_fat){
        return -1;
    }
    int entry_offset =fat_offset%512;
    read_sector(fat_sector, fat_table);
    unsigned int dest_cluster = merge_bytes((uint8_t*)&fat_table[entry_offset], 4);
    dest_cluster=dest_cluster&0x0FFFFFFF;
    kfree(fat_table);
    return dest_cluster;
}
int modify_fat(uint32_t cluster, uint32_t value){
    if(cluster<2) return -1;
    unsigned char* fat_table = kmalloc(512);
    if(!fat_table) return -1;
    int fat_offset=cluster*4;
    int fat_sector = partition_start+reserved_sectors +(fat_offset/512);
    if(fat_sector>partition_start+reserved_sectors+sectors_per_fat){
        return -1;
    }
    int entry_offset =fat_offset%512;
    read_sector(fat_sector, fat_table);
    fat_table[entry_offset]=((value) & 0xFF);
    fat_table[entry_offset+1]=((value>>8) & 0xFF);
    fat_table[entry_offset+2]=((value>>16) & 0xFF);
    fat_table[entry_offset+3]=((value>>24) & 0xFF);
    for(int i=0; i<num_of_fats; i++){
        if(write_sector(fat_sector+i*sectors_per_fat, fat_table)==-1) printf("FAILED WRITE");
    }
    kfree(fat_table);
    return 0;
}

DirectoryListing directory_parse(int cluster_num){ //caller must free result.entries
    DirectoryListing result = {0};
    int size=0;
    uint8_t* cluster = kmalloc(sectors_per_cluster*512);
    if(!cluster) return result;
    int sector_num=sector_of_cluster(cluster_num);
    int entries_per_cluster = sectors_per_cluster*512/sizeof(Cluster_Entry);
    while(cluster_num<0x0FFFFFF8 && cluster_num>=2){ //signals end of dir/file
        sector_num=sector_of_cluster(cluster_num);
        for(int i=0; i<sectors_per_cluster; i++){
            read_sector(sector_num+i, &cluster[512*i]);
        }
        size++;
        result.entries = krealloc(result.entries, size*sectors_per_cluster*512);
        if(!result.entries) return result;
        int i=0;
        while(cluster[i]!='\0' && i<((entries_per_cluster)*sizeof(Cluster_Entry))){
            memcpy(&result.entries[result.count], &cluster[i],sizeof(Cluster_Entry));
            result.count++;
            i+=sizeof(Cluster_Entry);
        }
        cluster_num=get_from_fat(cluster_num);
    }
    kfree(cluster);
    return result;
}
char* filename_to_plaintext(unsigned char *filename){ //caller must free file_plaintext
    to_uppercase(filename);
    char* file_plaintext = kmalloc(FAT_NAME_LENGTH+2); //+2 for null terminator and period
    if(!file_plaintext) return NULL;
    memset(file_plaintext, 0, FAT_NAME_LENGTH+2);
    int name_len=0;
    for(int i=0; i<8; i++){
        if(filename[i]!=' '){
            file_plaintext[name_len]=filename[i];
            name_len++;
        }
    }
    if(filename[8]!=' ' || filename[9]!=' ' || filename[10]!=' '){
        file_plaintext[name_len]='.';
        name_len++;
        for(int i=8; i<11; i++){
            if(filename[i]!=' '){
                file_plaintext[name_len]=filename[i];
                name_len++;
            }
        }
    }
    return file_plaintext;
}
char* file_contents(char* filename){
    int clusternum = 0;
    int bytes_written = 0;
    File_Location location = get_file_location(filename, FIND_EXISTS);
    char* contents_buffer = NULL;
    if(location.byte_offset==-1){
        return NULL;
    }
    uint8_t *file_sector = kmalloc(512);
    if(!file_sector) return NULL;
    read_sector(location.lba, file_sector);
    Cluster_Entry* file_entry = (Cluster_Entry*)&file_sector[location.byte_offset];
    if((file_entry->attr&0x10) == 0x10){
        return NULL;
    }
    char* sectordata = kmalloc(512);
    uint32_t current_cluster = (uint32_t)file_entry->low_cluster_number+((uint32_t)file_entry->high_cluster_number<<16);
    while(current_cluster>=2 && current_cluster<0x0FFFFFF8){
        contents_buffer=krealloc(contents_buffer, sectors_per_cluster*512*(1+clusternum));
        if(!contents_buffer) return NULL;
        int current_sector = sector_of_cluster(current_cluster);
        for(int i=0; i<sectors_per_cluster; i++){
            read_sector(current_sector+i, sectordata);
            for(int j=0; j<512; j++){
                if(sectordata[j]=='\0') continue;
                bytes_written++;
                contents_buffer[(clusternum*sectors_per_cluster*512)+(i*512)+j]=sectordata[j];
            }
        }
        clusternum++;
        if(clusternum>=256){
            kfree(sectordata);
            return NULL;
        }
        current_cluster=get_from_fat(current_cluster);
    }
    kfree(sectordata);
    contents_buffer[bytes_written]='\0';
    return contents_buffer;
}
unsigned char* plaintext_to_filename(char* filename){ //caller must free file_fatname
    if(strlen(filename)>12){
        return NULL;
    }
    if(strlen(filename)==0) return NULL;
    to_uppercase(filename);
    unsigned char* file_fatname = kmalloc(FAT_NAME_LENGTH);
    if(!file_fatname) return NULL;
    memset(file_fatname, ' ', FAT_NAME_LENGTH);
    int extension_start=0;
    _Bool period_exists=0;
    _Bool valid_fat=0;
    for(int i=0; i<strlen(filename); i++){
        if(filename[i]=='.' && period_exists) return NULL;
        if(filename[i]=='.'){
            period_exists=1;
            extension_start=i+1;
        }
        if(!(filename[i]=='.' || filename[i]=='/' || filename[i]=='\\')) valid_fat=1;
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
    if (period_exists) {
        for(int i=0; i<3; i++){
            if(filename[extension_start+i]!='\0') file_fatname[8+i]=filename[extension_start+i];
        }
    }
    return file_fatname;
}
char* names_from_directory(DirectoryListing list){ //caller must free list
    int file_num=0;
    char* name_list = NULL;
    int total_length=0;
    int start_pos=0;
    for(int i=0; i<list.count; i++){
        if(list.entries[i].name[0]==0) continue;
        if(list.entries[i].name[0]==0xE5) continue; //skip over deleted files
        if((list.entries[i].attr & 0b00001111) == 0b00001111) continue; //skip over long file names
        char* plain_filename = filename_to_plaintext(list.entries[i].name);
        if(!plain_filename) return NULL;
        total_length+=(strlen(plain_filename)+1);
        name_list = krealloc(name_list, total_length);
        if(!name_list) return NULL;
        memcpy(&name_list[start_pos],plain_filename, strlen(plain_filename));
        name_list[total_length-1]=' ';
        start_pos=total_length;
        kfree(plain_filename);
    }
    if(name_list) name_list[total_length-1]='\0';
    return name_list;
}
int file_path_destination(char* input_dir){
    if(input_dir[0] !='/'){
        return -1; //absolute paths only
    }
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
            if(location_in_dirname>12){
                return -1;
            }
            current_location++;
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
        if(stack_top>= MAX_PATH_DEPTH){
            return -1;
        }
        cluster_stack[stack_top++]=current_cluster;
        DirectoryListing current_dir_results= directory_parse(current_cluster); 
        unsigned char* file_entry = plaintext_to_filename(nextdir);
        if(!file_entry){
            return -1;
        }
        memset(nextdir, 0, sizeof(nextdir));
        current_cluster = -1;
        for(int i=0; i<current_dir_results.count; i++){
            if(strncmp(current_dir_results.entries[i].name, file_entry, 11)==0 && ((current_dir_results.entries[i].attr & 0x10) != 0)){
                current_cluster=(uint32_t)current_dir_results.entries[i].low_cluster_number+((uint32_t)current_dir_results.entries[i].high_cluster_number<<16);
            }
        }
        kfree(file_entry);
        kfree(current_dir_results.entries);
        if(current_cluster==-1){
            return -1;
        }
    }
    return current_cluster;
}
int extend_file(int cluster){
    uint32_t new_cluster=0;
    uint32_t current_cluster=0;
    uint32_t prev_last_cluster=0;
    while(cluster>=2 && cluster<0x0FFFFFF8){
        prev_last_cluster=cluster;
        cluster=get_from_fat(cluster);
    }
    if(cluster!=0x0FFFFFF8) return 1;
    uint32_t first_fat_sector=partition_start+reserved_sectors;
    uint8_t *sectordata = kmalloc(512);
    for(int i=0; i<sectors_per_fat; i++){
        read_sector(first_fat_sector+i, sectordata);
        for(int j=0; j<512; j+=4){
            if(sectordata[j]==0 && sectordata[j+1]==0 && sectordata[j+2]==0 && sectordata[j+3]==0){
                goto skip;
            }
            current_cluster++;
        }
    }
    kfree(sectordata);
    return 1;
skip:
    kfree(sectordata);
    modify_fat(current_cluster, 0x0FFFFFF8);
    modify_fat(prev_last_cluster, current_cluster);
    uint8_t *null_sector = kmalloc(512);
    memset(null_sector, 0, 512);
    int start_sector=sector_of_cluster(current_cluster);
    for(int i=0;i<sectors_per_cluster; i++){
        write_sector(start_sector+i, null_sector);
    }
    kfree(null_sector);
    return 0;
}
char* append_path(char* filepath){ //caller must free
    if(filepath[0]=='/'){
        return strdup(filepath);
    }
    int wd_len=strlen(working_dir);
    int fp_len=strlen(filepath);
    if (fp_len==0) return strdup(working_dir);
    _Bool add_slash=!(working_dir[wd_len-1]=='/');
    char* new_filepath = kmalloc(fp_len+wd_len+add_slash+1);//1 for null terminator
    if(!new_filepath) return NULL;
    memcpy(new_filepath, working_dir, wd_len);
    if(add_slash)new_filepath[wd_len]='/';
    memcpy(new_filepath+wd_len+add_slash, filepath, fp_len);
    new_filepath[wd_len+add_slash+fp_len]='\0';
    return new_filepath;
}
File_Location get_file_location(char* filename, LookupMode mode){
    int slash=-1;
    File_Location not_found = {-1, -1, -1};
    int len = strlen(filename);
    char* filepath = NULL;
    _Bool match = 0;
    int lba_offset=0;
    int byte_offset=0;
    int cluster=0;
    File_Location filedest= {0};
    char* name=NULL;
    for(int i=0; i<len; i++){
        if(filename[i]=='/') slash=i;
    }
    if(slash>=0){
        filepath = kmalloc(slash+1);
        name = kmalloc(len+1-slash); 
        memcpy(filepath, filename, slash);
        filepath[slash]='\0';
        memcpy(name, &filename[slash+1], len-(slash+1));
        name[len-(slash+1)]='\0';
    }
    else{
        filepath = kmalloc(1);
        name = kmalloc(len+1);
        memcpy(name, filename, len);
        filepath[0]='\0';
        name[len]='\0';
    }
    char* full_path=append_path(filepath);
    if(!full_path){
        return not_found;
    }
    cluster = file_path_destination(full_path);
    if(cluster==-1){
        kfree(full_path);
        return not_found;
    }
    DirectoryListing dirlist = directory_parse(cluster);
    unsigned char* formatted_name = plaintext_to_filename(name);
    if(!formatted_name){
        kfree(full_path);
        return not_found;
    }
    for(int i=0; i<dirlist.count; i++){
        if(!dirlist.entries[i].name) continue;
        if(!formatted_name) break;
        if(mode==FIND_EXISTS){
            if(strncmp(dirlist.entries[i].name, formatted_name, 11)==0){
                match=1;
                break;
            }
        }
        byte_offset+=32;
        if (byte_offset==512){
            byte_offset=0;
            lba_offset++;
        }
        if (lba_offset==sectors_per_cluster){
            cluster=get_from_fat(cluster);
            if(cluster<2 || cluster>= 0x0FFFFFF8) break;
            lba_offset=0;
        }
    }
    kfree(name);
    kfree(dirlist.entries);
    kfree(full_path);
    kfree(formatted_name);
    kfree(filepath);
    filedest.lba = lba_offset+sector_of_cluster(cluster);
    filedest.cluster = cluster;
    filedest.byte_offset= byte_offset;
    if (match==0) {
        return not_found;
    }
    return filedest;
}
int delete_file(char* filename){
    File_Location location = get_file_location(filename, FIND_EXISTS);
    if(location.byte_offset==-1){
        return 1;
    }
    if(location.cluster<2){
        return 1;
    }
    uint8_t *file_sector = kmalloc(512);
    if(!file_sector) return 1;
    read_sector(location.lba, file_sector);
    Cluster_Entry* file_entry = (Cluster_Entry*)&file_sector[location.byte_offset];
    if(file_entry->name[0]=='.' && (file_entry->name[1]=='.' || file_entry->name[1]==' ')) return 1;
    if(file_entry->attr == 0b00001111){
        file_entry->name[0]=0xE5;
        write_sector(location.lba, file_sector);
        kfree(file_sector);
        return 0;
    }
    uint8_t attr =file_entry->attr;
    uint32_t cluster = (uint32_t)file_entry->low_cluster_number|((uint32_t)file_entry->high_cluster_number<<16);
    if((attr & 0b00010000)!=0){ //recursive removal for directories
        DirectoryListing list = directory_parse(cluster);
        int namelen = strlen(filename);
        for(int i=0; i<list.count; i++){
            if(list.entries[i].name[0]==0x00||list.entries[i].name[0]==0xE5 || (list.entries[i].attr & 0x0F) == 0x0F) continue;
            char* new_filename = filename_to_plaintext(list.entries[i].name);
            if(!new_filename) continue;
            if((strcmp(new_filename, ".")==0)||(strcmp(new_filename, "..")==0)){
                kfree(new_filename);
                continue;
            }
            int full_name_size = strlen(new_filename)+namelen+2;
            char* full_name = kmalloc(full_name_size);
            memset(full_name, 0, full_name_size);
            if(!full_name){
                kfree(new_filename);
                kfree(file_sector);
                return 1;
            }
            strcpy(full_name, filename);
            strcat(full_name, "/");
            strcat(full_name, new_filename);
            kfree(new_filename);
            delete_file(full_name);
            kfree(full_name);
        }
        kfree(list.entries);
    }
    while(cluster >=2 && cluster<0x0FFFFFF8){
        uint32_t next= get_from_fat(cluster);
        modify_fat(cluster, 0);
        cluster=next;
    }
    file_sector[location.byte_offset]=0xE5; //signifies deleted file
    write_sector(location.lba, file_sector);
    kfree(file_sector);
    return 0;
}
int check_attributes(char* filename){
    File_Location location = get_file_location(filename, FIND_EXISTS);
    if (location.byte_offset ==-1) return -1;
    uint8_t *file_sector = kmalloc(512);
    if(!file_sector) return -1;
    read_sector(location.lba, file_sector);
    Cluster_Entry* file_entry = (Cluster_Entry*)&file_sector[location.byte_offset];
    int attribute =  file_entry->attr;
    kfree(file_sector);
    return attribute;
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
int create_file(char* filename){
    File_Location location = get_file_location(filename, FIND_NEW);
    if(location.byte_offset!=0) delete_file(filename);


}