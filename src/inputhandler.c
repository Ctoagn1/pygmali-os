#include "keyboardhandler.h"
#include "commands.h"
#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "inputhandler.h"
#include "tty.h"
char previous_inputs_buffer[PREV_INPUTS_BUFFER]= {0};
typedef void (*command_function)(int argc, char argv[MAX_ARGS][ARG_SIZE]);
typedef struct{
    const char *name;
    command_function function;
} Command;

Command commands[] = {
    {"echo", cmd_echo},
    {"help", cmd_help},
    {"play", cmd_play_note},
    {"clear", cmd_clear},
    {"time", cmd_time},
    {NULL, NULL} //signals end of list
};

void store_prev_inputs(){
    size_t len = strlen(input_buffer);
    memmove(&previous_inputs_buffer[len+1], &previous_inputs_buffer[0], PREV_INPUTS_BUFFER-(len+1));
    memcpy(&previous_inputs_buffer[0], &input_buffer[0], len+1); //include null byte!
}

int parse_input(char argv[MAX_ARGS][ARG_SIZE]){ //returns number of args, writes to inputted memory
    uint16_t start=0;
    uint16_t end=0;
    uint8_t argnum = 0;
    store_prev_inputs();
    while(1){
        while(start<INPUT_BUFFER_SIZE && input_buffer[start] == ' '){
            start++;
        }
        end = start;
        if(input_buffer[start]== '\0') break;
        while(end<INPUT_BUFFER_SIZE && input_buffer[end] != ' ' && input_buffer[end] != '\0'){
            end++;
        }
        if((end-start)>=ARG_SIZE || argnum >= MAX_ARGS) {
            clear_input_buffer();
            return -1;
        }
        memcpy(argv[argnum], &input_buffer[start], end-start);
        argv[argnum][end-start]='\0'; //args are null terminated
        argnum++;
        start=end;
    }
    clear_input_buffer();
    return argnum;
}
int run_command(int argc, char argv[MAX_ARGS][ARG_SIZE]){
    if (argc==0){
        return 0;
    } 
    for(int i=0; commands[i].name != NULL; i++){
        if(strcmp(argv[0], commands[i].name)==0){
            commands[i].function(argc, argv);
            return 1;
        }
    }
    printf("Command \"%s\" not found.\n", argv[0]);
    return -1;
}
void clear_args(char argv[MAX_ARGS][ARG_SIZE]){
    for(int i=0; i<MAX_ARGS; i++){
        memset(argv[i], 0, ARG_SIZE);
    }
}
void parse_and_run(){
    char argv[MAX_ARGS][ARG_SIZE];
    int argc = parse_input(argv);
    is_input_from_user=0;
    run_command(argc, argv);
    clear_args(argv);
    terminal_shell_set();
}