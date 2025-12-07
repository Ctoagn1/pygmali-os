#include "process.h"
#include "kmalloc.h"
#define PROCESS_BITMAP_SIZE 0x18000

bool first_time = true;
ProcessNode* process_list;
ProcessNode* current_process;
Process process_table[MAX_PROCESSES];
int process_count = 0;
uint8_t pid_bitmap[512]={0};
const void *kernel_stacks_start=(void*)0xC0C00000;
const void *kernel_stacks_end=(void*)0xC0800001;

static inline void switch_page_directory(uint32_t pd) {
    asm volatile("mov %0, %%cr3" :: "r"(pd));
}
void initialize_scheduling(){
    char* code = kmalloc(1);
    *code = 0xf4; //hlt
    char* name = kmalloc(7);
    *name = "SYSTEM";
    create_process(code, 1, name);

}

void create_process(char* contents, int bytesize, char* name){
    Process p;
    p.name = strdup(name);
    for(int i=0; i<process_number; i++){
        if(pid_bitmap[i/8]&(1<<i%8)==0){
            p.pid=i;
            pid_bitmap[i/8]|=1<<i%8;
            p.kernel_stack_base =(uint32_t)kernel_stacks_start-i*USER_KERNEL_STACK_SIZE;
            break;
        }
    }
    uint32_t *old_cr3 = get_page_table_virtual(1023);
    p.page_directory = create_process_address_space();
    p.page_bitmap = kmalloc(PROCESS_BITMAP_SIZE);
    PageNode* endoflist = kmalloc(sizeof(PageNode));
    endoflist->next = NULL;
    endoflist->physical_page = 0;
    endoflist->virtual_page = 0;

    PageNode* endoftablelist = kmalloc(sizeof(PageNode));
    endoftablelist->next = NULL;
    endoftablelist->physical_page = 0;
    endoftablelist->virtual_page = 0;
    p.tablelist = endoftablelist;

    switch_page_directory(p.page_directory);
    memcpy(0, contents, bytesize);
    switch_page_directory(old_cr3);

    add_process(p);

}


void add_process(Process p) {
    process_count++;
    ProcessNode* node = kmalloc(sizeof(ProcessNode));
    node->p = p;
    if(!process_list){
        process_list=node;
        node->next= &node;
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
                    switch_page_directory(current_process->p.page_directory);
                }
            }
        }
        prev = cur;
        cur = cur->next;
    } while (cur != process_list);
    kfree(cur->p.name);

    PageNode* pagelist = cur->p.pagelist;
    while(pagelist->next!=NULL){ //pagelist is virtual so that it can support removing individual pages while keeping process active
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
    free_raw_page(cur->p.page_directory);
    kfree(cur);
    return;
}


void schedule(Regs registers){

    pit_timer();
    if(first_time){
        first_time=false;
        registers.cs=USER_CS;
        registers.eip=0;
        registers.eflags=EFLAGS_IF;
        registers.useresp = 0xBFFFFFFF;
    }
    current_process->p.registers = registers;
    current_process = current_process->next;
    updateTSSesp(current_process->p.kernel_stack_base);
    switch_page_directory(current_process->p.page_directory);
    context_switch(current_process->p.registers);
}