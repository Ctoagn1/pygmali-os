#ifndef PROCESSES
#define PROCESSES
#include "paging.h"
#include "gdt.h"
#include <stddef.h>
#include <stdbool.h>
#include "pit.h"
#include "string.h"
#define USER_CS 0x1B 
#define USER_DS 0x23 
#define EFLAGS_IF 0x202
#define MAX_PROCESSES 64
#define USER_KERNEL_STACK_SIZE 0x2000
#define process_number 4096



typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} Regs;

typedef struct PageNode{
    uint32_t physical_page;
    uint32_t virtual_page;
    struct PageNode* next;
} PageNode;

typedef struct{
    uint32_t* page_directory;
    uint32_t kernel_stack_base;
    Regs registers;
    int pid;
    char* name;
    PageNode* pagelist;
    PageNode* tablelist;
    uint8_t* page_bitmap;
} Process;

typedef struct ProcessNode{
    Process p;
    struct ProcessNode* next;
} ProcessNode;



void create_process(char* contents, int bytesize, char* name);
void add_process(Process p);
void kill_process(int pid);
void restore_process_state(Process* p);
void schedule(Regs registers);
#endif