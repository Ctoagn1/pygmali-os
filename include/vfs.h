#ifndef VFS
#define VFS
#include <stddef.h>
#include "kmalloc.h"
#include "errno.h"
#include "string.h"
#define VFS_TYPE_LEN 32
#define VFS_PATH_LEN 256
#define MAX_MOUNTPOINTS 12
#define MAX_TOKENS 64
#define VNODE_CACHE_SIZE 2048
#define DENTRY_CACHE_SIZE 2048
#define MAX_FILENAME 64
#define FS_DRIVER_COUNT 16

struct vfs_node; 
struct fs_ops;
struct dentry;
struct fs_instance_t;
struct vfs_node_list;
struct mountpoint_t;
struct path_token_t;
struct path_tokens_t;

#define VNODE_FILE 1
#define VNODE_DIR 2
#define VNODE_SYMLINK 3
#define VNODE_CHARDEV 4
#define VNODE_BLOCKDEV 5


typedef struct dentry{ 
    uint64_t parent;
    uint64_t target;
    char filename[MAX_FILENAME];
    size_t namelen;
    struct dentry* next;
} dentry;


typedef struct fs_dirent{
    uint64_t inode;
    int64_t offset;
    uint16_t reclen;
    uint8_t type;
    char d_name[];
} fs_dirent;

typedef struct stat_info{
    uint64_t id;
    uint32_t type;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t blocks;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
} stat_info;


struct fs_driver;

typedef struct fs_instance_t{
    struct fs_driver* driver;
    uint64_t root_inode_id;
    void* superblock;
    void* device;
    void* root_inode;
    struct vfs_node_list* vnode_cache[VNODE_CACHE_SIZE];
    dentry* dentry_cache[DENTRY_CACHE_SIZE];
    int flags;
}fs_instance_t;


typedef struct vfs_node{
    int references;
    uint64_t id;
    void* fs_internal;
    struct fs_instance_t* fs;
    stat_info attributes;
    size_t refcount;
} vfs_node;


typedef struct vfs_node_list{
    vfs_node* node;
    struct vfs_node_list* next;
} vfs_node_list;

typedef struct fs_ops{
    int(*read)(vfs_node* node, void* buf, size_t size, size_t offset);
    int(*write)(vfs_node* node, const void* buf, size_t size, size_t offset);
    uint64_t (*lookup)(vfs_node* dir, const char* name, size_t len);
    int(*create)(vfs_node* dir, const char* name, uint32_t type, uint64_t* out_inode);
    int(*unlink)(vfs_node* dir, const char* name);
    int(*mkdir)(vfs_node* dir, const char* name, uint32_t mode, uint64_t* inode);
    void*(*iget)(fs_instance_t* fs, uint64_t id);
    int (*getattr)(vfs_node* file, stat_info* info);
    int (*getdents)(vfs_node* dir, fs_dirent* buf, size_t len);
} fs_ops;

typedef struct mountpoint_t{
    char mountpoint[VFS_PATH_LEN];
    fs_instance_t* fs_instance;

    struct mountpoint_t* next;
} mountpoint_t;

typedef struct {
    const char* start;
    size_t len;
} path_token_t;

typedef struct fs_driver{
    const char* name;
    fs_instance_t* (*mount)(void* source);
    int(*probe)(void* device, size_t size);
    int (*umount)(fs_instance_t* fs);
    const fs_ops* ops;
} fs_driver;

typedef struct {
    path_token_t tokens[MAX_TOKENS];
    int count;
} path_tokens_t;


void tokenize_path(const char* path, path_tokens_t* out);
uint32_t cache_hash(uint64_t a);
uint64_t dentry_hash(uint64_t id, const char* name, size_t len);
fs_instance_t* vfs_mount(const char* target, void* device, const fs_driver* fs);
int vfs_umount(const char *target);
int is_prefix_mount(const char* path, const char* mnt);
vfs_node* resolve(const char* path);
int create_dentry(fs_instance_t* fs, uint64_t parent, uint64_t target, const char* name, size_t name_len);
int64_t resolve_dentry(fs_instance_t* fs, uint64_t parent, const char* filename, size_t filename_len);


int vfs_mkdir(vfs_node* dir, char* filename, uint32_t mode, uint64_t* out_inode);
int vfs_read(vfs_node* node, void* buf, size_t count, size_t offset);
int vfs_write(vfs_node* node, const void* buf, size_t count, size_t offset);
int vfs_getdents(vfs_node* dir, fs_dirent* out, size_t size);
int vfs_create(vfs_node* dir, char* filename, uint32_t type, uint64_t* out_inode);
vfs_node* vfs_lookup(vfs_node* dir, const char* name);

void put_vnode(vfs_node* node);
vfs_node* get_vnode(fs_instance_t* fs_instance, uint64_t id);
#endif