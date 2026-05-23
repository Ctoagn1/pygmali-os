
#include "process.h"
#include "fd.h"
#include "syscalls.h"
#include "io.h"
#include "fatparser.h"
#include "kmalloc.h"
extern ProcessNode* current_process;

#define KERNEL_BASE 0xC0000000

/*
void syscall_handler(Regs* regs){
    int call_num = regs->eax;
    int returnval=0;
    switch(call_num){
        case 0x3:
            returnval=sys_read(regs->ebx, (void*)regs->ecx, regs->edx);
            if (current_process->p.state==PROC_BLOCKED) schedule(regs);
            break;
        case 0x4:
            returnval=sys_write(regs->ebx, (void*)regs->ecx, regs->edx);
            break;
        case 0x5:
            returnval=sys_open((char*)regs->ebx, regs->ecx, regs->edx);
            break;
        case 0x58:
            reboot();
        default:
            ;
        asm volatile (
        "mov %0, %%eax"
        :
        : "r"(returnval)
    :   "%eax"
);
        
    }
    return;

}
int sys_read(int fd, void *buf, int size) {
    Process *p = &current_process->p;
    if (fd < 0 || fd >= MAX_FD) return -1;

    File *f = p->fd_table[fd];
    if (!f || !f->fileops || !f->fileops->read) return -1;

    return f->fileops->read(f, buf, size);
}

int sys_write(int fd, const void *buf, int size) {
    Process *p = &current_process->p;
    if (fd < 0 || fd >= MAX_FD) return -1;

    File *f = p->fd_table[fd];
    if (!f || !f->fileops || !f->fileops->write) return -1;

    return f->fileops->write(f, buf, size);
}

bool is_user_address(uint32_t addr, uint32_t len){
    if(addr>=KERNEL_BASE) return false;
    if(addr+len-1>=KERNEL_BASE) return false;
    return true;
}
int sys_open(const char* filename, int flags, int mode){
    File* newfd = kmalloc(sizeof(File));
    newfd->flags = flags;
    newfd->offset = 0;
    newfd->reference_count = 1;
    if(assign_filedata(newfd, filename)==-1){
        kfree(newfd);
        return -1;
    }
    Process *p = &current_process->p;
    fd_alloc(p, newfd);
    return 0;
}

void reboot()
{
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
    asm volatile("hlt");
}
*/