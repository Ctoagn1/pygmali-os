
#include "ramfs.h"



const fs_ops ramfs_ops = {
    .create = ramfs_create,
    .unlink = ramfs_unlink,
    .read = ramfs_read,
    .write = ramfs_write,
    .getattr = ramfs_stat,
    .lookup = ramfs_lookup,
    .getdents = ramfs_getdents,
    .iget = ramfs_iget
};

const fs_driver ramfs_driver = {
    .ops = &ramfs_ops,
    .mount = create_ramsfs,
    .umount = NULL,
    .probe = NULL,
    .name = "ramfs"
    
};

void* ramfs_iget(fs_instance_t* fs, uint64_t id){
    ramfs_superblock *sb = fs->superblock;
    return sb->inode_map[id];
}
fs_instance_t* create_ramsfs(void* source){
    (void) source;
    fs_instance_t* instance = kmalloc(sizeof(fs_instance_t));
    if(!instance) goto err3;
    ramfs_superblock* superblock = kmalloc(sizeof(ramfs_superblock));
    if(!superblock) goto err2;
    ramfs_node* root = kmalloc(sizeof(ramfs_node));
    if(!root) goto err1;

    memset(superblock, 0, sizeof(ramfs_superblock));
    memset(root, 0, sizeof(ramfs_node));

    instance->superblock = superblock;
    instance->device = NULL;
    instance->driver = &ramfs_driver;
    instance->flags = 0;
    instance->root_inode_id=1;
    instance->root_inode = root;
    
    superblock->root = root;
    superblock->id_count = 1;
    superblock->inode_map[1]=root;

    root->children=NULL;
    root->superblock=superblock;
    root->data=NULL;
    root->size=0;
    root->type = VNODE_DIR;
    root->id = superblock->id_count++;
    return instance;
    err1:
        kfree(superblock);
    err2:
        kfree(instance);
    err3:
        return NULL;
}

uint64_t ramfs_lookup(vfs_node* vnode, const char* filename, size_t len){
    ramfs_node*dir = (ramfs_node*)vnode->fs_internal;
    ramfs_dirent* it = dir->children;
    while(it){
        if(len == it->namelen && strncmp(filename, it->name, len)==0)return it->target->id;
        it=it->next;
    }
    return -ENOENT;
}
int ramfs_create(vfs_node* vnode, const char* filename, uint32_t type, uint64_t* inode ){
    ramfs_node*parent = (ramfs_node*)vnode->fs_internal;
    size_t len = strlen(filename);
    ramfs_superblock* block = parent->superblock;
    if(len>=MAX_FILENAME) return -ENAMETOOLONG;
    if((int64_t)ramfs_lookup(vnode, filename, len)!=-ENOENT) return -EEXIST;

    ramfs_node* new_node = kmalloc(sizeof(ramfs_node));
    if(!new_node) return -ENOMEM;
    memset(new_node, 0, sizeof(ramfs_node));
    ramfs_dirent* new_entry = kmalloc(sizeof(ramfs_dirent));
    if(!new_entry){
        kfree(new_node);
        return -ENOMEM;
    }
    memset(new_entry, 0, sizeof(ramfs_dirent));
    new_node->id = block->id_count;
    new_node->type = type;
    new_node->linkcount = 1;
    new_node->size=0;
    new_node->superblock = block;
    new_node->data=NULL;
    block->inode_map[block->id_count]=new_node;
    block->id_count++;

    memcpy(new_entry->name, filename, len);
    new_entry->target = new_node;
    new_entry->namelen = len;
    new_entry->next = parent->children;
    parent->children = new_entry;
    *inode = new_node->id;
    return 0;
}
int ramfs_read(vfs_node* vnode, void* buf, size_t size, size_t offset){
    ramfs_node*inode = (ramfs_node*)vnode->fs_internal;
    if(!inode) return -EBADF;
    if(offset>=inode->size) return 0;
    uint64_t copied_bytes = (inode->size-offset) > size ? size : (inode->size-offset);
    memcpy(buf, inode->data+offset, copied_bytes);
    return copied_bytes;
}
int ramfs_write(vfs_node* vnode, const void* buf, size_t size, size_t offset){
    ramfs_node*inode = (ramfs_node*)vnode->fs_internal;
    if(!inode) return -EBADF;
    if(offset+size>inode->size){
        size_t new_size = offset+size;
        void* tmp=krealloc(inode->data, new_size);
        if(!tmp) return -ENOMEM;
        inode->data=tmp;
        inode->size = new_size;
    }
    memcpy(&inode->data[offset], buf, size);
    return size;
}
int ramfs_stat(vfs_node* vnode, stat_info* info){
    ramfs_node*inode = (ramfs_node*)vnode->fs_internal;
    info->id = inode->id;
    info->type = inode->type;
    info->mode = 0;
    info->uid = 0;
    info->gid = 0;
    info->size = inode->size;
    info->blocks = inode->size; //just size for now
    info->atime = 0;
    info->ctime = 0;
    info->mtime = 0;
    return 0;
}
int ramfs_getdents(vfs_node* vnode, fs_dirent* buf, size_t bufsize){
    ramfs_node*inode = (ramfs_node*)vnode->fs_internal;
    size_t offset = 0; //ignore offset for now
    if(!inode) return -EBADF;
    uint64_t used_bytes = 0;
    ramfs_dirent* child = inode->children;
    for(unsigned int i=0; i<offset; i++){
        child=child->next;
        if(!child) return 0;
    }
    while(child){
        size_t namesize = child->namelen+1;
        if(used_bytes+namesize+sizeof(fs_dirent)>bufsize) return 0;
        buf->inode=child->target->id;
        buf->reclen=namesize+sizeof(fs_dirent);;
        buf->type=child->target->type;
        buf->offset=0; //ignore for now
        memcpy(buf->d_name, child->name, namesize-1);
        buf->d_name[namesize-1]='\0';
        buf = (fs_dirent*)((uint8_t*)buf+namesize+sizeof(fs_dirent));
        child=child->next;
        used_bytes+=namesize+sizeof(fs_dirent);
    } 
    return 0;
}
int ramfs_unlink(vfs_node* vnode, const char* name){
    size_t len = strlen(name);
    ramfs_node* inode = (ramfs_node*)vnode->fs_internal;
    ramfs_dirent* it = inode->children;
    ramfs_dirent* prev = NULL;
    while(it){
        if(len == it->namelen && strncmp(name, it->name, len)==0){
            ramfs_node* child = it->target;
            if(child->type==VNODE_DIR) return -EISDIR;
            if(!prev)inode->children = it->next;
            else prev->next = it->next;
            child->linkcount--;
            if(child->linkcount==0){
                kfree(child->data);
                kfree(child);
            }

            kfree(it);
            return 0;   
        }
        prev=it;
        it=it->next;
    }
    return -ENOENT;
}
