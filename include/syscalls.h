#include "process.h"
#include "fd.h"
#include "io.h"

int sys_read(int fd, void *buf, int size);
int sys_write(int fd, const void *buf, int size);
bool is_user_address(uint32_t addr, uint32_t len);
int sys_open(const char* filename, int flags, int mode);
void reboot();
void syscall_handler(Regs* regs);