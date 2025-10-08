#include "keyboardhandler.h"
#include "fatparser.h"
#include "string.h"
#include "tty.h"
#include "pit.h"
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
void syscall_print_to_screen(char* string){
    terminal_writestring(string);
}
void syscall_print_to_screen_at(char* string, int x, int y){
    int oldrow = terminal_row;
    int oldcol = terminal_column;
    terminal_row = y;
    terminal_column = x;
    terminal_writestring(string);
    terminal_row=oldrow;
    terminal_column=oldcol;
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

