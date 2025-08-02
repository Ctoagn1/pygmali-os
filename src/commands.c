#include "tty.h"
#include "pit.h"
#include "string.h"
#include "printf.h"
#include "inputhandler.h"
#include "rtc.h"
#include "fatparser.h"
void cmd_echo(int argc, char** argv){
    for(int i=1; i<argc; i++){
        terminal_writestring(argv[i]);
        terminal_putchar(' ');
    }
    terminal_putchar('\n');
}

void cmd_help(int argc, char** argv){
    terminal_writestring("Commands- echo <word>, ls <optional directory>, pwd, cd <directory>, play <note> <duration>, time, clear, help <enter a command here for info>\n");
}
void cmd_play_note(int argc, char** argv){
    if(argc < 3){
        printf("Usage: play <note, accidentals written like ASHARP4> <duration in ms>\n");
        return;
    }
    _Bool valid_note = 0;
    int frequency = 0;
    int interval = 0;
    for(int i=0; i<sizeof(note_table)/sizeof(Note); i++){
        if(strcmp(argv[1], note_table[i].name)==0){
            frequency = note_table[i].frequency;
            valid_note =1;
        }
    }
    interval = str_to_int(argv[2]);
    if(!valid_note){
        printf("Note %s not valid.\n", argv[1]);
        return;
    }
    if(interval>10000){
        printf("Time %s not valid.\n", argv[2]);
        return;
    }
    play_sound(frequency, interval);
}
void cmd_clear(int argc, char** argv){
    terminal_initialize();
}
void cmd_time(int argc, char** argv){
    display_time();
}
void cmd_ls(int argc, char** argv){
    char* filepath = strdup(argv[1]);
    if(argc<2) filepath="";
    char* full_path = append_path(filepath);
    kfree(filepath);
    int cluster = file_path_destination(full_path);
    if(cluster==-1){
        printf("Directory \"%s\" not found.\n", full_path);
        kfree(full_path);
        return;
    }
    DirectoryListing dir_contents = directory_parse(cluster);
    char* list_of_names = names_from_directory(dir_contents);
    printf("%s", list_of_names);
    kfree(filepath);
    kfree(dir_contents.entries);
    kfree(list_of_names);
}
void cmd_pwd(int argc, char** argv){
    printf("%s\n", working_dir);
}
void cmd_cd(int argc, char** argv){
    if(argc<2){
        printf("Usage: cd <directory>\n");
        return;
    }
    char* full_path=append_path(argv[1]);
    if(!full_path) return;
    normalize_path(full_path);
    if(file_path_destination(full_path)!=-1){
        int length=strlen(full_path);
        if(full_path[length-1]=='/' && length!=1)  full_path[length-1]='\0';
        char* old = working_dir;
        working_dir = full_path;
        if (old) kfree(old);
    }
    else{
        printf("Directory \"%s\" not found.\n", full_path);
        kfree(full_path);
    }
}