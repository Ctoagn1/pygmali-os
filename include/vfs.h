#ifndef VFS
#define VFS
#include <stddef.h>
#include "kmalloc.h"
#include "string.h"
#define VFS_TYPE_LEN 32
#define VFS_PATH_LEN 256
#define MAX_MOUNTPOINTS 12
#define MAX_TOKENS 64
#define VNODE_CACHE_SIZE 2048
#define DENTRY_CACHE_SIZE 2048
#define MAX_FILENAME 64


struct vfs_node; 
struct fs_ops;
struct dentry;
struct fs_instance_t;
struct vfs_node_list;
struct mountpoint_t;
struct path_token_t;
struct path_tokens_t;

typedef enum {
    VNODE_FILE,
    VNODE_DIR,
    VNODE_SYMLINK,
    VNODE_CHARDEV,
    VNODE_BLOCKDEV,
} vnode_type_t;

typedef struct vnode_attr_t{
    uint32_t flags;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    vnode_type_t type;
} vnode_attr_t;


typedef struct dentry{
    struct vfs_node* parent;
    struct vfs_node* target;
    char filename[MAX_FILENAME];
    size_t namelen;
    struct dentry* next;
} dentry;

typedef struct fs_instance_t{
    struct fs_ops* ops;
    void* superblock;
    struct vfs_node_list* vnode_cache[VNODE_CACHE_SIZE];
    dentry* dentry_cache[DENTRY_CACHE_SIZE];
    void* device;
    struct vfs_node* root;
    int flags;
    void* private_data;
}fs_instance_t;


typedef struct vfs_node{
    int references;
    uint64_t id;
    struct fs_instance_t* fs;
    void* fs_internal;
    vnode_attr_t attributes;
    struct vfs_node* parent;
} vfs_node;


typedef struct vfs_node_list{
    vfs_node* node;
    struct vfs_node_list* next;
} vfs_node_list;





typedef struct fs_ops{
    int(*read)(void* vfs_node, void* buf, size_t size, size_t offset);
    int(*write)(void* vfs_node, const void* buf, size_t size, size_t offset);
    uint64_t (*lookup)(vfs_node* dir, const char* name, size_t len);
    int(*create)(vfs_node* dir, const char* name, int type);
    int(*unlink)(vfs_node* dir, const char* name);
    int (*getattr)(vfs_node* file, vnode_attr_t* info);
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

typedef struct {
    path_token_t tokens[MAX_TOKENS];
    int count;
} path_tokens_t;


void tokenize_path(const char* path, path_tokens_t* out);
uint32_t cache_hash(uint64_t id);
uint64_t dentry_hash(uint64_t id, const char* name, size_t len);
int vfs_mount(const char* target, fs_instance_t* fs);
int vfs_umount(const char *target);
int is_prefix_mount(const char* path, const char* mnt);
vfs_node* resolve(const char* path);
int create_dentry(vfs_node* parent, vfs_node* target, const char* name, size_t name_len, fs_instance_t* fs);
vfs_node* resolve_dentry(vfs_node* parent, const char* filename, size_t filename_len, fs_instance_t* fs);
vfs_node* get_vnode(fs_instance_t* fs_instance, uint64_t id);
#endif