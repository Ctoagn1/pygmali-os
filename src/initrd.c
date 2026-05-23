#include "vfs.h"

#define CPIO_S_IFMT   0170000
#define CPIO_S_IFREG  0100000
#define CPIO_S_IFDIR  0040000

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

uint32_t parse_hex(char* s, int len){
    uint32_t result = 0;
    for(int i=0; i<len; i++){
        result<<=4;
        char c = s[i];
        if(c>='0' && c<='9') result |= c - '0';
        else if (c >= 'A' && c <= 'F')result |= c - 'A' + 10;
        else return UINT32_MAX;
    }
    return result;
}



int load_cpio_into_ramfs(void* archive, fs_instance_t* ramfs, uint32_t size){

    uint8_t* p = (uint8_t*)archive;
    uint8_t* end = p + size;
    if(p+sizeof(cpio_newc_header)>end) return -EINVAL;
    path_tokens_t* toks = kmalloc(sizeof(path_tokens_t));
    while(1){
        cpio_newc_header* header = (cpio_newc_header*)p;
        if(!header) return -1;
        if(strncmp(header->c_magic, "070701", 6) != 0) return -EINVAL;
        uint32_t namesize = parse_hex(header->c_namesize, 8);
        uint32_t filesize = parse_hex(header->c_filesize, 8);
        uint32_t mode = parse_hex(header->c_mode, 8);
        uint32_t type = mode & CPIO_S_IFMT;
        if(namesize>VFS_PATH_LEN) return -1;
        char* name = (char*)(p+sizeof(*header));
        uint8_t* filedata = (uint8_t*)ALIGN4((uintptr_t)(name+namesize));
        if(strcmp(name, "TRAILER!!!")==0) break;

        tokenize_path(name, toks);
        
        vfs_node* cur = get_vnode(ramfs, ramfs->root_inode_id);
        if(!cur) return -1;

        for(int i=0; i<toks->count-1; i++){

            char component[MAX_FILENAME+1];
            memcpy(component, toks->tokens[i].start, toks->tokens[i].len);
            component[toks->tokens[i].len]='\0';
            vfs_node* next = vfs_lookup(cur, component);
            if(!next){
                uint64_t ino;
                int err = vfs_mkdir(cur, component, 0, &ino);
                if(err){
                    put_vnode(cur);
                    return err;
                }
                next=get_vnode(ramfs, ino);
                if(!next){
                    put_vnode(cur);
                    return -1;
                }
            }
            put_vnode(cur);
            cur = next;
        }
        char filename[MAX_FILENAME+1];
        path_token_t* last = &toks->tokens[toks->count-1];
        if(toks->count == 0){ 
            return -EINVAL;
        }
        memcpy(filename, last->start, last->len);
        filename[last->len]='\0';
        
        vfs_node* file = vfs_lookup(cur, filename);
        if(!file){
            uint64_t ino;
            int err;
            if(type==CPIO_S_IFDIR) err = vfs_mkdir(cur, filename, mode, &ino);
            else if(type==CPIO_S_IFREG) err = vfs_create(cur, filename, mode, &ino);
            else continue;

            if(err){
                put_vnode(cur);
                return err;
            }
            file = get_vnode(ramfs, ino);
            if(!file){
                put_vnode(cur);
                return -1;
            }
        }
        if(type==CPIO_S_IFREG){
            int written = vfs_write(file, filedata, filesize, 0);
            if(written<0) return written;
        }
        put_vnode(file);
        put_vnode(cur);
        p=(uint8_t*)ALIGN4((uintptr_t)(filedata + filesize));
        if(p>end)return -EINVAL;
    }
    kfree(toks);
    return 0;
        
}