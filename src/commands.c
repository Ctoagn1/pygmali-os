#include "tty.h"
#include "pit.h"
#include "string.h"
#include "printf.h"
#include "inputhandler.h"
#include "rtc.h"
#include "fatparser.h"
void cmd_echo(int argc, char** argv){
    for(int i=1; i<=argc; i++){
        terminal_writestring(argv[i]);
        terminal_putchar(' ');
    }
    terminal_putchar('\n');
}

void cmd_help(int argc, char** argv){
    terminal_writestring("Commands-\necho <what you want to repeat>, help, play <note, C1-B6, accidentals must be written as note-SHARP-octave, like ASHARP4> <duration in ms, cap is 10000\n");
}
void cmd_play_note(int argc, char** argv){
    if(argc < 3){
        printf("Usage: play <note> <duration>\n");
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
    char* filepath = argv[1];
    filepath = append_path(filepath);
    int cluster = file_path_destination(filepath);
    DirectoryListing dir_contents = directory_parse(cluster);
    char* list_of_names = names_from_directory(dir_contents);
    int length = strlen(list_of_names);
    for(int i=0; i<length/11; i++){
        for(int j=0; j<11; j++){
            if(j==9) terminal_putchar('.');
            if(list_of_names[i*11+j]!=' ') terminal_putchar(list_of_names[i*11+j]);
        }
        terminal_putchar('\n');
    }
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
    normalize_path(full_path);
    if(!full_path) return;
    if(file_path_destination(full_path)!=-1){
        int length=strlen(full_path);
        if(full_path[length-1]=='/')  full_path[length-1]='\0';
        if(working_dir) kfree(working_dir);
        char* old = working_dir;
        working_dir = full_path;
        if (old) kfree(old);
    }
    else{
        kfree(full_path);
        printf("Directory not found.\n");
    }
}