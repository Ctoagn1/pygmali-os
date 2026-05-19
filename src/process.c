#include "process.h"
#include "kmalloc.h"
#include "tty.h"
#include "fatparser.h"
#include "fd.h";
#define PROCESS_BITMAP_SIZE 0x18000
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10


File stdin_file = {NULL, 0, 0, 0, NULL};
File stdout_file = {NULL, 0, 0, 0, NULL};
bool first_time = true;
ProcessNode* process_list;
ProcessNode* current_process;
int process_count = 0;
uint8_t pid_bitmap[512]={0};
const void *kernel_stacks_start=(void*)0xC0C00000;
const void *kernel_stacks_end=(void*)0xC0800001;

static inline void switch_page_directory(uint32_t pd) {
    asm volatile("mov %0, %%cr3" :: "r"(pd) : "memory");
}
void initialize_scheduling(){
    FileOperations* tty_ops = kmalloc(sizeof(FileOperations));
    tty_ops->read = stdin_read;
    tty_ops->write = stdout_write;
    tty_ops->close = NULL;
    stdin_file.fileops=tty_ops;
    stdout_file.fileops=tty_ops;
    char* name = kmalloc(5);
    memcpy(name, "IDLE", 5);

    create_process(name, (uint32_t)idle_task);
    Regs* procregs = &process_list->p.registers;
    procregs->useresp = process_list->p.kernel_stack_top;
    procregs->cs = KERNEL_CS;
    procregs->ss = KERNEL_DS;


}

Process* create_process(const char* name, uintptr_t entry){
    asm volatile("cli");
    Process p;
    p.name = strdup(name);
    p.block_reason=BLOCK_NONE;
    for(int i=0; i<PROCESS_NUM; i++){
        if((pid_bitmap[i/8]&(1<<i%8))==0){
            p.pid=i;
            pid_bitmap[i/8]|=1<<i%8;
            p.kernel_stack_top =(uint32_t)kernel_stacks_start-i*USER_KERNEL_STACK_SIZE;
            alloc_page(p.kernel_stack_top);
            alloc_page(p.kernel_stack_top-4096);
            break;
        }
    }

    uint32_t *old_cr3 = get_page_table_virtual(1023);
    old_cr3 = (uint32_t*)phys_from_virt(old_cr3);
    p.page_directory = create_process_address_space();
    p.page_bitmap = kmalloc(PROCESS_BITMAP_SIZE);
    p.state = PROC_NEW;

    memset(&p.registers, 0, sizeof(Regs));
    p.registers.cs = USER_CS;
    p.registers.ss = USER_DS;
    p.registers.eip = entry;
    p.registers.useresp = 0xBFFFFFFF;
    p.registers.eflags = EFLAGS_IF | 0x200;
    p.fd_table[0]= &stdin_file;
    stdin_file.reference_count++;
    p.fd_table[1]= &stdout_file;
    stdout_file.reference_count++;
    memset(p.page_bitmap, 0xFF, PROCESS_BITMAP_SIZE);
    PageNode* endoflist = kmalloc(sizeof(PageNode));
    endoflist->next = NULL;
    endoflist->physical_page = 0;
    endoflist->virtual_page = 0;
    p.pagelist = endoflist;


    PageNode* endoftablelist = kmalloc(sizeof(PageNode));
    endoftablelist->next = NULL;
    endoftablelist->physical_page = 0;
    endoftablelist->virtual_page = 0;
    p.tablelist = endoftablelist;
    add_process(p);
    switch_page_directory(p.page_directory.phys);
    switch_page_directory((uint32_t)old_cr3);

    asm volatile("sti");
    return &p;
}

void idle_task(){
    while(1){
        asm volatile("hlt");
    }
}
void add_process(Process p){
    process_count++;
    ProcessNode* node = kmalloc(sizeof(ProcessNode));
    node->p = p;
    if(!process_list){
        process_list=node;
        node->next= node;
        current_process=node;
    }
    else{
        ProcessNode* tail = process_list;
        while(tail->next != process_list){
            tail = tail->next;
        }
        tail->next=node;
        node->next = process_list;
    }
}

void kill_process(int pid) {
    if (!process_list) return;
    ProcessNode* prev = process_list;
    ProcessNode* cur = process_list;
    do {
        if (cur->p.pid == pid) {
            if (cur == prev) {
                process_list = NULL;
                current_process = NULL;
            } else {
                prev->next = cur->next;
                if (cur == process_list) {
                    process_list = cur->next;
                }
                if (cur == current_process) {
                    current_process = cur->next;
                    switch_page_directory(current_process->p.page_directory.phys);
                }
            }
            break;
        }
        prev = cur;
        cur = cur->next;
    } while (cur != process_list);
    kfree(cur->p.name);
    int fd_count = 0;
    while(cur->p.fd_table[fd_count]!=NULL && fd_count<MAX_FD){
        cur->p.fd_table[fd_count]->reference_count--;
        if(cur->p.fd_table[fd_count]->reference_count==0){
            kfree(cur->p.fd_table[fd_count]);
        }
        fd_count++;
    }
    PageNode* pagelist = cur->p.pagelist;
    while(pagelist->next!=NULL){
        free_raw_page(pagelist->physical_page);
        PageNode* next = pagelist->next;
        kfree(pagelist);
        pagelist = next;
    }
    kfree(pagelist);

    PageNode* tablelist = cur->p.tablelist;
    while(tablelist->next!=NULL){
        free_raw_page(tablelist->physical_page);
        PageNode* next = tablelist->next;
        kfree(tablelist);
        tablelist = next;
    }
    kfree(tablelist);
    free_page(cur->p.page_directory.virt);
    kfree(cur);
    return;
}

void schedule(Regs* registers){

    if(current_process->p.state==PROC_NEW){
        *registers = current_process->p.registers;
    }
    if(current_process->p.state=PROC_RUNNING) current_process->p.state=PROC_READY;
    memcpy(&(current_process->p.registers), registers, sizeof(Regs));

    do{
        current_process = current_process->next;
    }while(current_process->p.state==PROC_BLOCKED);

    current_process->p.state=PROC_RUNNING;

    updateTSSesp(current_process->p.kernel_stack_top);
    switch_page_directory(current_process->p.page_directory.phys);
    memcpy(registers, &(current_process->p.registers), sizeof(Regs));
}
void wake_process(Process* p, block_reason_t reason){
    if(p->state != PROC_BLOCKED) return;
    if(p->block_reason != reason) return;
    p->state = PROC_READY;
    p->block_reason = BLOCK_NONE;
}