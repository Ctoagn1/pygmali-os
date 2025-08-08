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
    terminal_writestring("Commands- echo <word>, ls <optional directory>, pwd, cd <directory>, cat <file>, play <note> <duration>, time, clear, touch <file>, mkdir <directory>, help <enter a command here for info>\n");
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
    char* filepath = NULL;
    if(argc<2) filepath = strdup("");
    else filepath = strdup(argv[1]);
    if(!filepath) return;
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
    if(!list_of_names) return;
    printf("%s", list_of_names);
    kfree(full_path);
    kfree(dir_contents.entries);
    kfree(list_of_names);
}
void cmd_pwd(int argc, char** argv){
    printf("%s\n", working_dir);
}
void cmd_cd(int argc, char** argv){
    if(argc<2){
        printf("Usage: cd <directory>, changes working directory\n");
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
void cmd_rm(int argc, char** argv){
    _Bool recursive=0;
    if(argc<2 || (argc==2 && strcmp(argv[1], "-r")==0)){
        printf("Usage: rm <file>, deletes target. Must use \"rm -r\" to delete directories.\n");
        return;
    }
    if(strcmp(argv[1], "-r")==0) recursive=1;
    if ((check_attributes(argv[argc-1]) & 0b10010000) == 0b00010000 && recursive==0){ //1 in 128 place to prevent -1 from registering as dir
        printf("Use \"rm -r\" if you're sure you want to delete a directory.\n");
        return;
    }
    if(delete_file(argv[argc-1])==-1) printf("File \"%s\" not found.\n", argv[argc-1]);
    else printf("Deleted %s ", argv[argc-1]);
}
void cmd_cat(int argc, char** argv){
    if(argc<2){
        printf("Usage: cat <file>, displays file contents to terminal.\n");
        return;
    }
    char* file = file_contents(argv[1]);
    if(!file){
        printf("File \"%s\" not found, is empty, is a directory, or is too large.\n", argv[1]);
        return;
    }
    printf(file);
    kfree(file);
    
}
void cmd_touch(int argc, char** argv){
    if(argc<2){
        printf("Usage: touch <file>, creates a file with given name. Name can only be up to 8 characters, extensions up to 3.");
    }
    if(create_file(argv[1], FILE)==-1) printf("Invalid filepath or filename. Names can be 8 characters, extensions 3. For example, output11.txt works, but output1234 does not.");
}
void cmd_mkdir(int argc, char** argv){
        if(argc<2){
        printf("Usage: mkdir <directory>, creates a directory with given name. Name can only be up to 8 characters, extensions up to 3.");
    }
    if(create_file(argv[1], DIRECTORY)==-1) printf("Invalid filepath or directory name. Names can be 8 characters, extensions 3.");
}