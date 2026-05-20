#ifndef RAMFS
#define RAMFS
#include <stdint.h>
#include "kmalloc.h"
#include <string.h>
#include "vfs.h"
#include <stddef.h>
#define NAME_LEN 128
#define MAX_RAMFS_FILESIZE 1<<32
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
}  cpio_newc_header;


struct ramfs_superblock;

typedef struct ramfs_node{
    size_t name_len;
    uint64_t id;
     
    struct ramfs_node* parent;
    struct ramfs_dirent* children;
    struct ramfs_superblock* superblock;

    vnode_type_t type;
    uint8_t* data;
    size_t size;
} ramfs_node;


struct ramfs_dirent;
typedef struct ramfs_dirent{
    char name[NAME_LEN];
    size_t namelen;
    ramfs_node* target;
    struct ramfs_dirent* next;
}ramfs_dirent;
typedef struct ramfs_superblock{
    ramfs_node* root;
    uint64_t id_count;
}ramfs_superblock;

uint32_t parse_hex(char* s, int len);
int load_ramfs_from_cpio(void* archive, fs_instance_t* ramfs);
ramfs_node* ramfs_lookup(ramfs_node* dir, char* filename, size_t len);
ramfs_node* create_node(ramfs_node* parent, char* filename, size_t len, ramfs_superblock* block);
#endif