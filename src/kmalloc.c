#include "kmalloc.h"
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "printf.h"

void *heap_start=NULL;
void *heap_end=NULL;
int allocs;
typedef struct block_header{
    size_t size;
    int free;
    struct block_header* next;
} block_header;
block_header* heap_base=NULL;


void* kmalloc(size_t size){
    size=ALIGN4(size);
    block_header* current = heap_base;
    block_header* prev = NULL;
    if(!current){ //first allocation
        heap_base =(block_header*) heap_start;
        heap_base->size = size;
        heap_base->free = 0;
        heap_base->next = NULL;
        //printf("Allocated %p ", heap_base+1);
        allocs++;
        return (void*)(heap_base+1); //return space after header
    }
    while(current && !(current->free && current->size >= size)){
        prev = current;
        current = current->next;
    }
    if(current){
        current->free=0; //set block as used
        //split if big enough
        if(current->size>=sizeof(block_header)+size+MIN_BLOCK_SIZE){
           block_header* split_block=(block_header*)((char*)current+size+sizeof(block_header));
           split_block->size=((current->size)-sizeof(block_header)-size);
           split_block->free=1;
           split_block->next=current->next;
           current->size=size;
           current->next=split_block;
        }
        //printf("Allocated %p ", current+1);
        allocs++;
        return (void*)(current+1);
    }
    //create new block
    if(prev==NULL)  return NULL;
    block_header* new_block = (block_header*)((char*)prev+sizeof(block_header)+prev->size); //pointer arithmetic is based off of data size so to add (size) bytes it is necessary to cast the pointer as a 1-byte data type
    if((uintptr_t)new_block + sizeof(block_header)+size > (uintptr_t)heap_end){
        printf("FULL %d:%p", size, new_block);
        return NULL; //cannot allocate further
    }
    new_block->size = size;
    new_block->free = 0;
    new_block->next = NULL;
    prev->next = new_block;
    //printf("Allocated %p ", new_block+1);
    allocs++;
    return (void*)(new_block+1);
}
void kfree(void* to_be_freed){
    if(!to_be_freed || ((char*)to_be_freed<(char*)heap_start || ((char*)to_be_freed)>((char*)heap_end)-sizeof(block_header))){
        printf("INVALID FREE %p", to_be_freed);
        return;
    }
    allocs--;
    //printf("Freed %p ", to_be_freed);
    block_header* header = ((block_header*)to_be_freed)-1;
    header->free = 1;
    block_header* current=heap_base;
    while(current && current->next){;
        //merging
        if(current->free && current->next->free){
            current->size+=(current->next->size)+sizeof(block_header);
            current->next=current->next->next;
        }
        else{
            current=current->next;
        }
    }
}
void* krealloc(void* ptr, size_t size){
    if(ptr == NULL){
        return kmalloc(size);
    }
    if(size == 0){
        kfree(ptr);
        return NULL;
    }
    block_header* header = ((block_header*)ptr)-1;
    size=ALIGN4(size);
    if(header->size >= size){
        return ptr;
    }
    void* new_ptr = kmalloc(size);
    if(!new_ptr) return NULL;
    memcpy(new_ptr, ptr, header->size);
    kfree(ptr);
    return new_ptr;
}
Heap_Info heap_stats(){
    int used_blocks=0;
    int used_bytes=0;
    int total_blocks=0;
    int total_bytes=0;
    Heap_Info stats = {0};
    block_header* current= (block_header*)heap_start;
    while(current!=NULL){
        total_blocks++;
        total_bytes+=current->size;
        if(current->free==0){
            used_blocks++;
            used_bytes+=current->size;
        }
        current=current->next;
    }
    stats.used_blocks=used_blocks;
    stats.used_bytes=used_bytes+used_blocks*sizeof(block_header);
    stats.total_blocks=total_blocks;
    stats.total_bytes=total_bytes+total_blocks*sizeof(block_header);
    stats.allocs=allocs;
    return stats;
}