#include "vfs.h"
#include "ramfs.h"
struct mountpoint_t* root_mountpoint;

fs_driver* driver_array[FS_DRIVER_COUNT];
void tokenize_path(const char* path, path_tokens_t* out) {
    int i = 0;
    out->count = 0;

    while (path[i]) {

        while (path[i] == '/'){
            i++;
        }

        if (!path[i]) break;

        int start = i;

        while (path[i] && path[i] != '/'){
            i++;
        }

        int len = i - start;

        out->tokens[out->count].start = &path[start];
        out->tokens[out->count].len = len;
        out->count++;
    }
}

uint32_t cache_hash(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x%VNODE_CACHE_SIZE;
}

uint64_t dentry_hash(uint64_t id, const char* name, size_t len){
    for(size_t i=0; i<len; i++){
        id^= (unsigned char)name[i];
        id*= 1099511628211ULL;
    }
    return id%DENTRY_CACHE_SIZE;
}


fs_instance_t* vfs_mount(const char* target, void* device, const fs_driver* driver){
    fs_instance_t* fs = driver->mount(device);
    mountpoint_t *new_mountpoint = kmalloc(sizeof(mountpoint_t));
    if(new_mountpoint == NULL) return NULL;
    new_mountpoint->fs_instance = fs;
    strcpy(new_mountpoint->mountpoint, target);
    mountpoint_t* list = root_mountpoint;
    while(1){
        if(list==NULL){
            root_mountpoint=new_mountpoint;
            break;
        }
        else if(list->next==NULL){
            list->next=new_mountpoint;
            break;
        }
        list=list->next;
    }
    return fs;
}

int vfs_umount(const char *target){
    mountpoint_t* list = root_mountpoint;
    mountpoint_t* last = NULL;
    while(1){
        if(list==NULL){
            return -1;
        }
        if(strcmp(list->mountpoint, target)==0){
            if(last==NULL){
                root_mountpoint=list->next;
                kfree(list);
                return 0;
            }
            else{
                last->next = list->next;
                kfree(list);
                return 0;
            }
        }
        last=list;
        list=list->next;
    }
    if(list->next==NULL) return -1;
}

int is_prefix_mount(const char* path, const char* mnt) {
    int i = 0;
    while (mnt[i]) {
        if (path[i] != mnt[i]) return 0;
        i++;
    }
    return (path[i] == '/' || path[i] == '\0');
}

mountpoint_t* get_mountpoint(const char* path){
    mountpoint_t* cur = root_mountpoint;
    mountpoint_t* best = NULL;
    int best_len = 0;
    while(cur){
        if(is_prefix_mount(path, cur->mountpoint)){
            int len = strlen(cur->mountpoint);
            if(len > best_len){
                best = cur;
                best_len = len;
            }
        }
        cur = cur->next;
    }
    return best;
}
#define PATH_STACK_SIZE 64
vfs_node* resolve(const char* path){
    uint64_t stack[64];
    int sp = 0;
    mountpoint_t* mount = get_mountpoint(path);
    if(!mount) return NULL;
    const char* fs_path = &path[strlen(mount->mountpoint)];
    if(*fs_path == '\0') fs_path = "/";
    path_tokens_t* pathnodes = kmalloc(sizeof(path_tokens_t));
    if(!pathnodes) return NULL;
    tokenize_path(fs_path, pathnodes);
    uint64_t current = mount->fs_instance->root_inode_id;
    
    path_token_t* tok = pathnodes->tokens;
    for(int i=0; i<pathnodes->count; i++){
        if(tok[i].len==1 && *tok[i].start=='.'){
            continue;
        }
        if(tok[i].len==2 && *tok[i].start=='.' && *(tok[i].start+1)=='.'){;
            if(sp>0) current = stack[--sp];
            continue;
        }
        int64_t next = resolve_dentry(mount->fs_instance, current, tok[i].start, tok[i].len);
        if(next == -ENOENT){
            vfs_node* dir = get_vnode(mount->fs_instance, current);
            next = mount->fs_instance->driver->ops->lookup(dir, tok[i].start, tok[i].len);
            put_vnode(dir);
            if(next==-ENOENT) goto err; 
            create_dentry(mount->fs_instance, current, next, tok[i].start, tok[i].len);
        }
        if(sp>=PATH_STACK_SIZE) goto err;
        stack[sp++] = current;
        current = next;
    }
    kfree(pathnodes);
    return get_vnode(mount->fs_instance, current);
    err:
    kfree(pathnodes);
    return NULL;

}
int delete_dentry(vfs_node* parent, const char* filename, size_t filename_len){
    uint64_t hash = dentry_hash(parent->id, filename, filename_len);
    dentry* iter = parent->fs->dentry_cache[hash];
    dentry* prev = NULL;
    while(iter){
        if(iter->parent == parent->id && iter->namelen == filename_len && strncmp(iter->filename, filename, filename_len)==0){
            if(prev==NULL) parent->fs->dentry_cache[hash]=iter->next;
            else prev->next=iter->next;
            kfree(iter);
            return 0;
        }
        prev=iter;
        iter=iter->next;
    }
    return -ENOENT;
}
int create_dentry(fs_instance_t* fs, uint64_t parent_id, uint64_t child_id, const char* name, size_t name_len){
    uint64_t hash = dentry_hash(parent_id, name, name_len);
    dentry* iter = fs->dentry_cache[hash];
    while(iter){
        if(iter->parent == parent_id && iter->namelen == name_len && strncmp(iter->filename, name, name_len)==0){
            return -EEXIST;
        }
        iter=iter->next;
    }
    dentry* new_dentry = kmalloc(sizeof(dentry));
    if(!new_dentry) return -ENOMEM;
    memcpy(new_dentry->filename, name, name_len);
    new_dentry->filename[name_len]='\0';
    new_dentry->namelen = name_len;
    new_dentry->parent = parent_id;
    new_dentry->target = child_id;
    new_dentry->next = fs->dentry_cache[hash];
    fs->dentry_cache[hash]=new_dentry;
    return 0;
}

int64_t resolve_dentry(fs_instance_t* fs, uint64_t parent, const char* filename, size_t filename_len){
    uint64_t hash = dentry_hash(parent, filename, filename_len);
    dentry* dent = fs->dentry_cache[hash];
    while(dent){
        if(dent->parent==parent && dent->namelen == filename_len && strncmp(dent->filename, filename, filename_len)==0)
            return dent->target;
        dent=dent->next;
    }
    return -ENOENT;
}
void put_vnode(vfs_node* node){
    if(node->refcount>0)
    node->refcount--;
    return;
}
vfs_node* get_vnode(fs_instance_t* fs_instance, uint64_t id){
    int hash = cache_hash(id);
    vfs_node_list* iter = fs_instance->vnode_cache[hash];
    while(iter){
        if(iter->node->id==id){
            iter->node->refcount++;
            return iter->node;
        }
        iter=iter->next;
    }
    vfs_node_list* new_entry = kmalloc(sizeof(vfs_node_list));
    if(!new_entry) return NULL;
    vfs_node* new_node = kmalloc(sizeof(vfs_node));
    if(!new_node){
        kfree(new_entry);
        return NULL;
    }
    new_entry->node = new_node;
    new_node->id = id;
    new_node->refcount = 1;
    new_node->fs = fs_instance;
    new_node->fs_internal = fs_instance->driver->ops->iget(fs_instance, id);
    if(!new_node->fs_internal)goto err;
    stat_info attr = {0};
    if(fs_instance->driver->ops->getattr){
        int code = fs_instance->driver->ops->getattr(new_entry->node, &attr);
        if(code!=0) goto err;
    }
    memcpy(&new_entry->node->attributes, &attr, sizeof(stat_info));
    new_entry->next = fs_instance->vnode_cache[hash];
    fs_instance->vnode_cache[hash]=new_entry;
    
    return new_entry->node;

    err:
        kfree(new_node);
        kfree(new_entry);
        return NULL;
}
int vfs_read(vfs_node* node, void* buf, size_t count, size_t offset){
    if(!node) return -1;
    if(node->attributes.type==VNODE_DIR) return -EISDIR;
    return node->fs->driver->ops->read(node, buf, count, offset);
}
int vfs_write(vfs_node* node, const void* buf, size_t count, size_t offset){
    if(!node) return -1;
    if(node->attributes.type==VNODE_DIR) return -EISDIR;
    return node->fs->driver->ops->write(node, buf, count, offset);
}
int vfs_getdents(vfs_node* dir, fs_dirent* out, size_t size){
    if(!dir) return -EBADF;
    if(dir->attributes.type!=VNODE_DIR) return -ENOTDIR;
    return dir->fs->driver->ops->getdents(dir, out, size);
    
}
int vfs_create(vfs_node* dir, char* filename, uint32_t type, uint64_t* inode ){
    (void) type;

    if(vfs_lookup(dir, filename)) return -EEXIST;
    return dir->fs->driver->ops->create(dir, filename, VNODE_FILE, inode);
}
int vfs_mkdir(vfs_node* dir, char* filename, uint32_t type, uint64_t* inode){
    (void)type;

    if(vfs_lookup(dir, filename)) return -EEXIST;
    return dir->fs->driver->ops->create(dir, filename, VNODE_DIR, inode);
}
vfs_node* vfs_lookup(vfs_node* dir, const char* name){
    int64_t ino = resolve_dentry(dir->fs, dir->id, name, strlen(name));
    if(ino==-ENOENT){
        ino = dir->fs->driver->ops->lookup(dir, name, strlen(name));
        if(ino<0) return NULL;

        create_dentry(dir->fs, dir->id, ino, name, strlen(name));
    }
    return get_vnode(dir->fs, ino);
}
int vfs_unlink(vfs_node* dir, const char* name){
    vfs_node* file = vfs_lookup(dir, name);
    if(!file) return -ENOENT;
    if(file->attributes.type==VNODE_DIR){
        put_vnode(file);
        return -EISDIR;
    }
    put_vnode(file);
    int err = dir->fs->driver->ops->unlink(dir, name);
    if(err)return err;
    delete_dentry(dir, name, strlen(name));
    return 0;
}