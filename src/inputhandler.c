#include "keyboardhandler.h"
#include "commands.h"
#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "inputhandler.h"
#include "tty.h"
#include "kmalloc.h"
#include "writingmode.h"
int historybuffersize = 4096;
char* previous_inputs_buffer= NULL;
int history_position = 0;
int history_current_size=0;
typedef void (*command_function)(int argc, char** argv);
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
    {"pwd", cmd_pwd},
    {"ls", cmd_ls},
    {"cd", cmd_cd},
    {"rm", cmd_rm},
    {"cat", cmd_cat},
    {"touch", cmd_touch},
    {"mkdir", cmd_mkdir},
    {NULL, NULL} //signals end of list
};
void store_prev_inputs(){
    size_t len = strlen(input_buffer);
    if(history_current_size+len+1>historybuffersize){
        historybuffersize+=((len+1)*5); //add more than needed to avoid constant reallocs
        previous_inputs_buffer=krealloc(previous_inputs_buffer, historybuffersize);
    }
    memmove(&previous_inputs_buffer[len+1], &previous_inputs_buffer[0], PREV_INPUTS_BUFFER-(len+1));
    memcpy(&previous_inputs_buffer[0], &input_buffer[0], len+1); // include null byte!
    history_current_size+=len+1;
}
void back_history(){
    clear_input_buffer();
    int i=history_position;
    int input_num=0;
    if(history_position>=history_current_size) return;
    while(previous_inputs_buffer[i]!='\0' && i<history_current_size){
        input_buffer[input_num]=previous_inputs_buffer[i];
        terminal_putchar(previous_inputs_buffer[i]);
        input_num++;
        i++;
    }
    history_position=i+1;
    update_cursor(terminal_column, terminal_row);
}
void forward_history(){
    clear_input_buffer();
    int i=history_position-2;
    int input_num=0;
    if(i<0) return;
    while(previous_inputs_buffer[i]!='\0' && i>=0){
        i--;
    }
    i++;
    history_position=i;
    while(previous_inputs_buffer[i]!='\0' && i<history_current_size){
        input_buffer[input_num]=previous_inputs_buffer[i];
        terminal_putchar(previous_inputs_buffer[i]);
        input_num++;
        i++;
    }
    update_cursor(terminal_column, terminal_row);
}

int parse_input(char ***argv_out){ // returns number of args, writes to *argv_out
    uint16_t start = 0;
    uint16_t end = 0;
    uint8_t argc = 0;
    char **argv = NULL;
    store_prev_inputs();
    while (1) {
        while (start < INPUT_BUFFER_SIZE && input_buffer[start] == ' ') {
            start++;
        }
        end = start;
        if (input_buffer[start] == '\0') break;

        while (end < INPUT_BUFFER_SIZE && input_buffer[end] != ' ' && input_buffer[end] != '\0') {
            end++;
        }

        int arg_len = end - start;
        char *arg = kmalloc(arg_len + 1); // +1 for null terminator
        if (!arg) goto fail;

        memcpy(arg, &input_buffer[start], arg_len);
        arg[arg_len] = '\0';

        char **new_argv = krealloc(argv, sizeof(char*) * (argc + 1));
        if (!new_argv) {
            kfree(arg);
            goto fail;
        }

        argv = new_argv;
        argv[argc++] = arg;
        start = end;
    }
    *argv_out = argv;
    clear_input_buffer();
    return argc;

fail:
    if (argv) {
        for (int i = 0; i < argc; i++) {
            kfree(argv[i]);
        }
        kfree(argv);
    }
    clear_input_buffer();
    return -1;
}

int run_command(int argc, char **argv){
    if (argc == 0 || !argv || !argv[0]) return 0;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].function(argc, argv);
            return 1;
        }
    }
    printf("Command \"%s\" not found.\n", argv[0]);
    return -1;
}

void clear_args(int argc, char*** argv){
    if (*argv) {
        for (int i = 0; i < argc; i++) {
            kfree((*argv)[i]);
        }
        kfree(*argv);
        *argv = NULL;
    }
}

void parse_and_run(){
    char **argv = NULL;
    int argc = parse_input(&argv);
    is_input_from_user = 0;
    history_position=0;
    run_command(argc, argv);
    clear_args(argc, &argv);
    scroll();
    terminal_shell_set();
}