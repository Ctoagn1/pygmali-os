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
#define PROCESS_NUM 4096

typedef enum {PROC_NEW, PROC_READY, PROC_RUNNING, PROC_BLOCKED} ProcessState;


typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} Regs;

typedef struct Page{
    uint32_t phys;
    uint32_t virt;
} Page;

typedef struct PageNode{
    uint32_t physical_page;
    uint32_t virtual_page;
    struct PageNode* next;
} PageNode;

typedef struct{
    Page page_directory;
    uint32_t kernel_stack_top;
    Regs registers;
    int pid;
    char* name;
    PageNode* pagelist;
    PageNode* tablelist;
    uint8_t* page_bitmap;
    ProcessState state;
} Process;

typedef struct ProcessNode{
    Process p;
    struct ProcessNode* next;
} ProcessNode;



void create_process(char* contents, int bytesize, char* name);
void add_process(Process p);
void kill_process(int pid);
void restore_process_state(Process* p);
void initialize_scheduling();
void schedule(Regs* registers);
#endif