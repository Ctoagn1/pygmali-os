#ifndef SYSCALLS
#define SYSCALLS
#include "keyboardhandler.h"
#include "fatparser.h"
#include "string.h"
#include "tty.h"
#include "pit.h"
#include <stdbool.h>
#define KERNEL_BASE 0xC0000000
typedef struct{
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pushed by pushad in wrapper
} RegStack;

void syscall_handler(RegStack* regs);
bool is_user_address(uint32_t addr, uint32_t len);
void syscall_get_key(void* dest, int buffersize);
void syscall_get_key_array(void* dest, int buffersize);
void syscall_print_rect(uint32_t* buffer, int topleft_x, int topleft_y, int x_len, int y_len);
void syscall_read_directory_names(char* absolute_filepath, void* dest, int buffersize);
void syscall_read(char* absolute_filepath, void* dest, int buffersize);
void syscall_write(char* contents, int bytesize, char* absolute_filepath);
void syscall_create_file(char* absolute_filepath);
void syscall_create_dir(char* absolute_filepath);
void syscall_delete(char* absolute_filepath);

#endif