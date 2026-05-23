#ifndef RAMFS
#define RAMFS
#include <stdint.h>
#include "kmalloc.h"
#include "string.h"
#include "vfs.h"
#include <stddef.h>
#define MAX_INODES 2048
#define NAME_LEN 128
#define MAX_RAMFS_FILESIZE 1<<20


struct ramfs_superblock;

typedef struct ramfs_node{
    uint64_t id;
    size_t refcount;
    size_t linkcount;
     
    struct ramfs_dirent* children;
    struct ramfs_superblock* superblock;

    uint32_t type;
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
    ramfs_node* inode_map[MAX_INODES];
}ramfs_superblock;

uint32_t parse_hex(char* s, int len);
int load_cpio_into_ramfs(void* archive, fs_instance_t* ramfs, uint32_t size);
uint64_t ramfs_lookup(vfs_node* dir, const char* filename, size_t len);
int ramfs_create(vfs_node* parent, const char* filename, uint32_t mode, uint64_t* out_inode);
int ramfs_stat(vfs_node* node, stat_info* info);
int ramfs_write(vfs_node* inode, const void* buf, size_t size, size_t offset);
int ramfs_read(vfs_node* node, void* buf, size_t size, size_t offset);
int ramfs_getdents(vfs_node* node, fs_dirent* buf, size_t bufsize);
void* ramfs_iget(fs_instance_t* fs, uint64_t id);
int ramfs_unlink(vfs_node* vnode, const char* name);
fs_instance_t* create_ramsfs(void* source);



extern const fs_ops ramfs_ops;
extern const fs_driver ramfs_driver;
#endif