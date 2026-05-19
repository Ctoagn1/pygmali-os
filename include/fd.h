#ifndef FILEDESCRIPTOR
#define FILEDESCRIPTOR
#include "process.h"

typedef struct Process Process;
#define MAX_FD 20
typedef struct File File;
typedef struct FileOperations FileOperations;
typedef struct Fat32_Data Fat32_Data;
struct FileOperations{
    int (*read)(File* f, void* buf, int n);
    int (*write)(File* f, const void* buf, int n);
    int (*close)(File* f);
};

struct File{
    FileOperations* fileops;
    int flags;
    int offset;
    int reference_count;
    Fat32_Data* data;
};
struct Fat32_Data{
    uint32_t start_cluster;
    uint32_t size;
    uint32_t current_cluster;
    uint32_t cluster_offset;
};


int file_close(File *f);
int close(File* f);
int fd_alloc(Process *p, File *f);
File *fd_get(Process *p, int fd);
int fd_close(Process *p, int fd);
int fd_open(Process *p, const char* filename, int flags, int mode);
#endif
