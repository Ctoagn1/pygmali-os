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

void syscall_handler(RegStack* regs){
    int call_num = regs->eax;
    void* dest = regs->ebx;
    int buffersize = regs->ecx;
    void* input_buffer = regs->edx;
    switch(call_num){
        case 0: 
            syscall_get_key(dest, buffersize);
        case 1:
            syscall_get_key_array(dest, buffersize);
        case 2:
            syscall_print_rect(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
        case 3:
            syscall_read_directory_names(input_buffer, dest, buffersize);
        case 4:
            syscall_read(input_buffer, dest, buffersize);
        case 5:
            syscall_write(dest, buffersize, input_buffer);
        case 6:
            syscall_create_file(input_buffer);
        case 7:
            syscall_create_dir(input_buffer);
        case 8:
            syscall_delete(input_buffer);
        default:
            ;
        
    }
    return;

}
_Bool is_user_address(uint32_t addr, uint32_t len){
    if(addr>=KERNEL_BASE) return false;
    if(addr+len-1>=KERNEL_BASE) return false;
    return true;
}


void syscall_get_key(void* dest, int buffersize){
    KeyEvent event = {0};
    get_keyevent(&event);
    memcpy(dest, &event, min(buffersize, sizeof(KeyEvent)));
    return;

}
void syscall_get_key_array(void* dest, int buffersize){ //for multiple keys at once, or non-ascii like ctrl. caller must free array
    _Bool* keyarray = kmalloc(KEYBOARD_SIZE*sizeof(_Bool));
    for(int i=0; i<sizeof(key_state)/sizeof(_Bool); i++){
        keyarray[i] = key_state[i];
    }
    memcpy(dest, &keyarray, min(buffersize, sizeof(KEYBOARD_SIZE*sizeof(_Bool))));
    kfree(keyarray);
    return;
}
void syscall_print_rect(uint32_t* buffer, int topleft_x, int topleft_y, int x_len, int y_len){
    uint32_t* membuff = (uint32_t*)virtual_framebuffer;
    uint32_t* startaddr = membuff+topleft_x+(topleft_y*selected_video_mode->width);
    for(int i=0; i<y_len; i++){
		for(int j=0; j<x_len; j++){
				startaddr[j+i*selected_video_mode->width]=buffer[j+i*x_len];
		}
	}
}
/*void syscall_read_directory_info(char* absolute_filepath, void* dest){ //must free list.entries
    int cluster = file_path_destination(absolute_filepath);
    DirectoryListing file_list = directory_parse(cluster);
    memcpy(dest, &file_list, file_list.count*sizeof(Cluster_Entry));
    kfree(file_list.entries);
}*/
void syscall_read_directory_names(char* absolute_filepath, void* dest, int buffersize){ //must free filenames
    int cluster = file_path_destination(absolute_filepath);
    if(cluster<2) return NULL;
    DirectoryListing file_list = directory_parse(cluster);
    char* names = names_from_directory(file_list);
    kfree(file_list.entries);
    memcpy(dest, names, min(buffersize, strlen(names)));
    kfree(names);
    return;
}
void syscall_read(char* absolute_filepath, void* dest, int buffersize){ //must free contents
    char* contents = file_contents(absolute_filepath);
    memcpy(dest, contents, min(buffersize, file_size_from_name(absolute_filepath)));
    kfree(contents);
    return;
}
void syscall_write(char* contents, int bytesize, char* absolute_filepath){
    write_to_file(contents, bytesize, absolute_filepath);
    return;
}
void syscall_create_file(char* absolute_filepath){
    create_file(absolute_filepath, FILE);
    return;
}
void syscall_create_dir(char* absolute_filepath){
    create_file(absolute_filepath, DIRECTORY);
    return;
}
void syscall_delete(char* absolute_filepath){
    delete_file(absolute_filepath);
    return;
}

