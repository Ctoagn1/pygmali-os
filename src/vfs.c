#include <stddef.h>
#include "kmalloc.h"
#include "string.h"
#define VFS_TYPE_LEN 32
#define VFS_PATH_LEN 64
#define MAX_MOUNTPOINTS 12
typedef struct{
    char type[VFS_TYPE_LEN];
    char device[VFS_PATH_LEN];
    char mountpoint[VFS_PATH_LEN];
    //fs_operations_t *operations;
} mountpoint_t;

struct mountpoint_node;

typedef struct mountpoint_node{
    mountpoint_t node;
    struct mountpoint_node* next
} mountpoint_node;
mountpoint_node* root = NULL;

int vfs_mount(char* device, char* target, char* fs_type){
    mountpoint_node *new_mountpoint = kmalloc(sizeof(mountpoint_t));
    if(new_mountpoint == NULL) return -1;
    strcpy(new_mountpoint->node.device, device);
    strcpy(new_mountpoint->node.type, fs_type);
    strcpy(new_mountpoint->node.mountpoint, target);
    mountpoint_node* list = root;
    while(1){
        if(list==NULL){
            root=new_mountpoint;
            break;
        }
        else if(list->next==NULL){
            list->next=new_mountpoint;
            break;
        }
        list=list->next;
    }
    if(list->next==NULL) return -1;
    else return 0;
}

int vfs_umount(char *device, char *target){
    mountpoint_node* list = root;
    mountpoint_node* last = NULL;
    while(1){
        if(list==NULL){
            return -1;
        }
        if(strcmp(list->node.device, device)==0 && strcmp(list->node.mountpoint, target)==0){
            if(last==NULL){
                root=list->next;
                kfree(list);
                return 0;
            }
            else{
                last->next = list->next;
                kfree(list);
                return 0;
            }
        }
    }
    if(list->next==NULL) return -1;
}
int similar_chars(char* a, char* b){
    int len=0;
    while(a[len] == b[len] && a[len]!='\0') len++;
    return len;

}
mountpoint_t* get_mountpoint(char * path){
    if(root==NULL) return NULL;
    mountpoint_node* list = root;
    mountpoint_t* mntpnt = NULL;
    int longest_path=0;
    while(list!=NULL){
        int path_len = similar_chars(path, list->node.mountpoint);
        if(path_len>longest_path){
            mntpnt = &list->node;
            longest_path=path_len;
        }
        list=list->next;
    }
    return mntpnt;
}