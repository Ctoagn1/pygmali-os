#include <stdint.h>
#include "kmalloc.h"
#include <string.h>
#include <stddef.h>
#define NAME_LEN 128
typedef struct{
 char c_magic[6];
 char c_ino[8];
 char c_mode[8];
 char c_uid[8];
 char c_gid[8];
 char c_nlink[8];
 char c_mtime[8];
 char c_filesize[8];
 char c_devmajor[8];
 char c_devminor[8];
 char c_rdevmajor[8];
 char c_rdevminor[8];
 char c_namesize[8];
 char c_check[8];
}  cpio_newc_header ;

typedef struct{
    char name[NAME_LEN];
    void* data;
    size_t size;
} file_entry;



typedef struct file_node{
    file_entry file;
    struct file_node* next;
} file_node;
file_node* ramfs=NULL;

uint32_t parse_hex(char* s, int len){
    uint32_t result = 0;
    for(int i=0; i<len; i++){
        result<<=4;
        char c = s[i];
        if(c>='0' && c<='9') result |= c - '0';
        else if (c >= 'A' && c <= 'F')result |= c - 'A' + 10;
    }
    return result;
}

void read_cpio(void* archive){
    uint8_t*p = archive;
    file_node* prev = NULL;
    while(1){
        cpio_newc_header* header = (cpio_newc_header*)p;
        if(strncmp(header->c_magic, "070701", 6) != 0) break;
        uint32_t namesize = parse_hex(header->c_namesize, 8);
        uint32_t filesize = parse_hex(header->c_filesize, 8);
        char* name = (char*)(p+sizeof(*header));
        if(strcmp(name, "TRAILER!!!")==0) break;

        uint8_t* filedata = (uint8_t*)ALIGN4((uintptr_t)(name+namesize));
        struct file_node* newfile = kmalloc(sizeof(file_node));
        if (ramfs==NULL) ramfs=newfile;
        memcpy(&newfile->file.name, name, namesize<NAME_LEN ? namesize : NAME_LEN);
        newfile->file.data = filedata;
        newfile->file.size = filesize;
        newfile->next = NULL;
        if(prev!=NULL){
            prev->next=newfile;
        }
        
        p = (uint8_t*)ALIGN4((uintptr_t)(filedata + filesize));
    }
}

