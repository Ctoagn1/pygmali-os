
#include "fatparser.h"
#include "fd.h"
#include "process.h"
#include "console.h"


#define MAX_FD 20



int file_close(File *f) {
    if (!f)
        return -1;

    f->reference_count--;
    if (f->reference_count > 0)
        return 0;

    if (f->fileops && f->fileops->close)
        return f->fileops->close(f);

    return 0;
}

int fd_alloc(Process *p, File *f) {
    for (int i = 0; i < MAX_FD; i++) {
        if (!p->fd_table[i]) {
            p->fd_table[i] = f;
            f->reference_count++;
            return i;
        }
    }
    return -1;
}

File *fd_get(Process *p, int fd) {
    if (fd < 0 || fd >= MAX_FD)
        return NULL;
    return p->fd_table[fd];
}

int fd_close(Process *p, int fd) {
    File *f = fd_get(p, fd);
    if (!f)
        return -1;

    p->fd_table[fd] = NULL;
    return file_close(f);
}
int close(File* f){
    kfree(f);
    return 0;
}
int fd_open(Process *p, const char* filename, int flags, int mode){
    File* newfd = kmalloc(sizeof(File));
    newfd->flags = flags;
    newfd->offset = 0;
    newfd->reference_count = 1;
    if(assign_filedata(newfd, filename)==-1){
        kfree(newfd);
        return -1;
    }
    if(fd_alloc(p, newfd)==-1){
        kfree(newfd);
        return -1;
    }
    return 0;
}



