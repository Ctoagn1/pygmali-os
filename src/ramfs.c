
#include "ramfs.h"
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

int load_ramfs_from_cpio(void* archive, fs_instance_t* ramfs){
    uint8_t*p = archive;
    ramfs_node* prev = NULL;
    if(!archive || !ramfs) return -1;
    ramfs_superblock* superblock = kmalloc(sizeof(ramfs_superblock));
    if(!superblock) return -1;
    ramfs_node* root = kmalloc(sizeof(ramfs_node));
    if(!root){
        kfree(superblock);
        return -1;
    }
    memset(root, 0, sizeof(ramfs_node));
    ramfs->private_data = superblock;
    superblock->root = root;
    superblock->id_count = 0;
    root->type = VNODE_DIR;
    root->id = superblock->id_count++;
    path_tokens_t* toks = kmalloc(sizeof(path_tokens_t));
    if(!toks){
        kfree(superblock);
        kfree(root);
        return -1;
    }

    while(1){
        ramfs_node* iter = root;
        cpio_newc_header* header = (cpio_newc_header*)p;
        if(strncmp(header->c_magic, "070701", 6) != 0) break;
        uint32_t namesize = parse_hex(header->c_namesize, 8);
        uint32_t filesize = parse_hex(header->c_filesize, 8);
        if(namesize>VFS_PATH_LEN || filesize > MAX_RAMFS_FILESIZE) break;
        char* name = (char*)(p+sizeof(*header));
        uint8_t* filedata = (uint8_t*)ALIGN4((uintptr_t)(name+namesize));
        if(strcmp(name, "TRAILER!!!")==0) break;
        tokenize_path(name, toks);
        for(int i=0; i<toks->count-1; i++){
            ramfs_node* next = ramfs_lookup(iter, toks->tokens[i].start, toks->tokens[i].len);
            if(!next){
                next = create_node(iter, toks->tokens[i].start, toks->tokens[i].len, superblock);
                next->type=VNODE_DIR;
            }
            iter=next;
        }
        ramfs_node* file = create_node(iter, toks->tokens[toks->count-1].start, toks->tokens[toks->count-1].len, superblock);
        file->data = kmalloc(filesize);
        memcpy(file->data, filedata, filesize);
        file->size = filesize;
        file->type = VNODE_FILE;

        p = (uint8_t*)ALIGN4((uintptr_t)(filedata + filesize));
    }
    kfree(toks);
    return 0;
}
ramfs_node* ramfs_lookup(ramfs_node* dir, char* filename, size_t len){
    ramfs_dirent* it = dir->children;
    while(it){
        if(len == it->namelen && strncmp(filename, it->name, len)==0)return it->target;
        it=it->next;
    }
    return NULL;
}
ramfs_node* create_node(ramfs_node* parent, char* filename, size_t len, ramfs_superblock* block){
    if(len>=MAX_FILENAME) return NULL;

    ramfs_node* new_node = kmalloc(sizeof(ramfs_node));
    if(!new_node) return NULL;
    memset(new_node, 0, sizeof(ramfs_node));
    ramfs_dirent* new_entry = kmalloc(sizeof(ramfs_dirent));
    if(!new_entry){
        kfree(new_node);
        return NULL;
    }
    memset(new_entry, 0, sizeof(ramfs_dirent));
    new_node->id = block->id_count++;
    new_node->parent = parent;

    memcpy(new_entry->name, filename, len);
    new_entry->target = new_node;
    new_entry->namelen = len;
    new_entry->next = parent->children;
    parent->children = new_entry;
    return new_node;
}