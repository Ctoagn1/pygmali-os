#ifndef PROCESSES
#define PROCESSES
#include "paging.h"
#include "gdt.h"
#include <stddef.h>
#include <stdbool.h>
#include "pit.h"
#include "fd.h"
#include "string.h"
#define USER_CS 0x1B 
#define MAX_FD 20
#define USER_DS 0x23 
#define EFLAGS_IF 0x202
#define MAX_PROCESSES 64
#define USER_KERNEL_STACK_SIZE 0x2000
#define PROCESS_NUM 4096

typedef struct File File;
typedef enum {PROC_NEW, PROC_READY, PROC_RUNNING, PROC_BLOCKED} ProcessState;
typedef enum {BLOCK_NONE=0, BLOCK_SLEEP, BLOCK_WAIT_INPUT, BLOCK_WAIT_TTY, BLOCK_WAIT_FD, BLOCK_WAIT_CHILD} block_reason_t;


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

typedef struct Process{
    Page page_directory;
    uint32_t kernel_stack_top;
    Regs registers;
    int pid;
    char* name;
    PageNode* pagelist;
    PageNode* tablelist;
    uint8_t* page_bitmap;
    ProcessState state;
    block_reason_t block_reason;
    File* fd_table[MAX_FD];
} Process;

typedef struct ProcessNode{
    Process p;
    struct ProcessNode* next;
} ProcessNode;

extern ProcessNode* current_process;

void idle_task();
Process* create_process(const char* name, uintptr_t entry);
void add_process(Process p);
void kill_process(int pid);
void restore_process_state(Process* p);
void initialize_scheduling();
void schedule(Regs* registers);
void wake_process(Process* p, block_reason_t reason);

#endif