#include "keyboardhandler.h"
#include "fatparser.h"
#include "string.h"
#include "tty.h"
#include "pit.h"
typedef struct{
    uint32_t cases, edi, esi, ebp, esp, ebx, edx, ecx, eax; //pushed by pushad in wrapper
} RegStack;

void syscall_handler(RegStack regs){
    switch(regs.eax){
        case 0: 
            
    }

}
KeyEvent syscall_get_key(){
    KeyEvent event = {0};
    get_keyevent(&event);
    return event;

}
_Bool* syscall_get_key_array(){ //for multiple keys at once, or non-ascii like ctrl. caller must free array
    _Bool* keyarray = kmalloc(KEYBOARD_SIZE*sizeof(_Bool));
    for(int i=0; i<sizeof(key_state)/sizeof(_Bool); i++){
        keyarray[i] = key_state[i];
    }
    return keyarray;
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
DirectoryListing syscall_read_directory_info(char* absolute_filepath){ //must free list.entries
    int cluster = file_path_destination(absolute_filepath);
    DirectoryListing file_list = directory_parse(cluster);
    return file_list;
}
char* sycall_read_directory_names(char* absolute_filepath){ //must free filenames
    int cluster = file_path_destination(absolute_filepath);
    if(cluster<2) return NULL;
    DirectoryListing file_list = directory_parse(cluster);
    char* names = names_from_directory(file_list);
    kfree(file_list.entries);
    return names;

}
char* syscall_read(char* absolute_filepath){ //must free contents
    char* contents = file_contents(absolute_filepath);
    return contents;
}
int syscall_write(char* contents, int bytesize, char* absolute_filepath){
    int returncode = write_to_file(contents, bytesize, absolute_filepath);
    return returncode;
}
int syscall_create_file(char* absolute_filepath){
    int returncode = create_file(absolute_filepath, FILE);
    return returncode;
}
int syscall_create_dir(char* absolute_filepath){
    int returncode = create_file(absolute_filepath, DIRECTORY);
    return returncode;
}
int syscall_delete(char* absolute_filepath){
    int returncode = delete_file(absolute_filepath);
    return returncode;
}

