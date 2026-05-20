#include "vfs.h"
struct mountpoint_t* root_mountpoint;
void tokenize_path(const char* path, path_tokens_t* out) {
    int i = 0;
    out->count = 0;

    while (path[i]) {

        while (path[i] == '/') i++;

        if (!path[i]) break;

        int start = i;

        while (path[i] && path[i] != '/') i++;

        int len = i - start;

        out->tokens[out->count].start = &path[start];
        out->tokens[out->count].len = len;
        out->count++;
    }
}

uint32_t cache_hash(uint64_t id){
    uint64_t y = id + 0x9E3779B97F4A7C15;
    y = (y^(y>>30)) * 0xBF58476D1CE4E5B9;
    y = (y^(y>>27)) * 0x94D049BB133111EB;
    return (y^(y>>31))%VNODE_CACHE_SIZE;
} 

uint64_t dentry_hash(uint64_t id, const char* name, size_t len){
    for(size_t i=0; i<len; i++){
        id^= (unsigned char)name[i];
        id*= 1099511628211ULL;
    }
    return id%DENTRY_CACHE_SIZE;
}


int vfs_mount(const char* target, fs_instance_t* fs){
    mountpoint_t *new_mountpoint = kmalloc(sizeof(mountpoint_t));
    if(new_mountpoint == NULL) return -1;
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
    return 0;
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
vfs_node* resolve(const char* path){
    mountpoint_t* mount = get_mountpoint(path);
    if(!mount) return NULL;
    const char* fs_path = &path[strlen(mount->mountpoint)];
    if(*fs_path == '\0') fs_path = "/";
    path_tokens_t* pathnodes = kmalloc(sizeof(path_tokens_t));
    if(!pathnodes) return NULL;
    tokenize_path(fs_path, pathnodes);
    int i=0;
    vfs_node* current = mount->fs_instance->root;
    vfs_node* parent = mount->fs_instance->root;
    path_token_t* tok = pathnodes->tokens;
    while(i<pathnodes->count){
        if(i<pathnodes->count-1 && current->attributes.type!=VNODE_DIR && current->attributes.type!=VNODE_SYMLINK){
            goto err;
        }
        if(tok[i].len==1 && *tok[i].start=='.'){
            i++;
            continue;
        }
        if(tok[i].len==2 && *tok[i].start=='.' && *(tok[i].start+1)=='.'){
            i++;
            if(current->parent)current=current->parent;
            continue;
        }
        parent = current;
        vfs_node* next = resolve_dentry(current, tok[i].start, tok[i].len, mount->fs_instance);
        if(!next){
            uint64_t id = mount->fs_instance->ops->lookup(current, tok[i].start, tok[i].len);
            if(id==0) goto err;
            current = get_vnode(mount->fs_instance, id);
            if(current==NULL){
                goto err;
            }   
            current->parent = parent;
            create_dentry(parent, current, tok[i].start, tok[i].len, mount->fs_instance);
            i++;
        }
        else{
            current=next;
            i++;
        }
    }
    kfree(pathnodes);
    return current;
    err:
    kfree(pathnodes);
    return NULL;

}

int create_dentry(vfs_node* parent, vfs_node* target, const char* name, size_t name_len, fs_instance_t* fs){
    uint64_t hash = dentry_hash(parent->id, name, name_len);
    dentry* iter = fs->dentry_cache[hash];
    while(iter){
        if(iter->parent == parent && iter->namelen == name_len && strncmp(iter->filename, name, name_len)==0){
            return -1;
        }
        iter=iter->next;
    }
    dentry* new_dentry = kmalloc(sizeof(dentry));
    if(!new_dentry) return -1;
    memcpy(new_dentry->filename, name, name_len);
    new_dentry->namelen = name_len;
    new_dentry->parent = parent;
    new_dentry->target = target;
    new_dentry->next = fs->dentry_cache[hash];
    fs->dentry_cache[hash]=new_dentry;
    return 0;
}
vfs_node* resolve_dentry(vfs_node* parent, const char* filename, size_t filename_len, fs_instance_t* fs){
    uint64_t hash = dentry_hash(parent->id, filename, filename_len);
    dentry* dent = fs->dentry_cache[hash];
    while(dent){
        if(dent->parent==parent && dent->namelen == filename_len && strncmp(dent->filename, filename, filename_len)==0)
            return dent->target;
        dent=dent->next;
    }
    return NULL;
}
vfs_node* get_vnode(fs_instance_t* fs_instance, uint64_t id){
    int hash = cache_hash(id);
    vfs_node_list* iter = fs_instance->vnode_cache[hash];
    while(iter){
        if(iter->node->id==id)return iter->node;
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
    new_node->fs = fs_instance;
    vnode_attr_t attr;
    int code = fs_instance->ops->getattr(new_entry->node, &attr);
    if(code!=0){
        kfree(new_node);
        kfree(new_entry);
        return NULL;
    }
    memcpy(&new_entry->node->attributes, &attr, sizeof(vnode_attr_t));
    new_entry->next = fs_instance->vnode_cache[hash];
    fs_instance->vnode_cache[hash]=new_entry;
    
    return new_entry->node;
}
int vfs_read(vfs_node* node, void* buf, size_t count, size_t offset){
    if(!node) return -1;
    if(node->attributes.type!=VNODE_FILE) return -1;
    return node->fs->ops->read(node, buf, count, offset);
}
int vfs_write(vfs_node* node, const void* buf, size_t count, size_t offset){
    if(!node) return -1;
    if(node->attributes.type!=VNODE_FILE) return -1;
    return node->fs->ops->write(node, buf, count, offset);
}
