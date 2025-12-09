#include "process.h"
#include "kmalloc.h"
#define PROCESS_BITMAP_SIZE 0x18000

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
    char* code = kmalloc(2);
    code[0] = 0xEB;
    code[1] = 0xFE; //endlessly jumps to itself
    char* name = kmalloc(7);
    memcpy(name, "SYSTEM", 7);
    create_process(code, 2, name);

}

void create_process(char* contents, int bytesize, char* name){
    asm volatile("cli");
    Process p;
    p.name = strdup(name);
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
    old_cr3 = phys_from_virt(old_cr3);
    p.page_directory = create_process_address_space();
    p.page_bitmap = kmalloc(PROCESS_BITMAP_SIZE);
    p.state = PROC_NEW;

    p.registers.cs = USER_CS;
    p.registers.ss = USER_DS;
    p.registers.eip = 0;
    p.registers.useresp = 0xBFFFFFFF;
    p.registers.eflags = EFLAGS_IF | 0x200;

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
    memcpy(0, contents, bytesize);
    switch_page_directory(old_cr3);

    asm volatile("sti");
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
    pit_timer();
    if(current_process->p.state==PROC_NEW){
        *registers = current_process->p.registers;
    }
    current_process->p.state=PROC_READY;
    memcpy(&(current_process->p.registers), registers, sizeof(Regs));

    do{
        current_process = current_process->next;
    }while(current_process->p.state==PROC_BLOCKED);

    current_process->p.state=PROC_RUNNING;

    updateTSSesp(current_process->p.kernel_stack_top);
    switch_page_directory(current_process->p.page_directory.phys);
    memcpy(registers, &(current_process->p.registers), sizeof(Regs));
}