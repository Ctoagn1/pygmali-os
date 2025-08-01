#include "keyboardhandler.h"
#include "commands.h"
#include <stdint.h>
#include "string.h"
#include "printf.h"
#include "inputhandler.h"
#include "tty.h"
#include "kmalloc.h"
#include "writingmode.h"
char previous_inputs_buffer[PREV_INPUTS_BUFFER]= {0};
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
    {NULL, NULL} //signals end of list
};

void store_prev_inputs(){
    size_t len = strlen(input_buffer);
    memmove(&previous_inputs_buffer[len+1], &previous_inputs_buffer[0], PREV_INPUTS_BUFFER-(len+1));
    memcpy(&previous_inputs_buffer[0], &input_buffer[0], len+1); // include null byte!
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
    run_command(argc, argv);
    clear_args(argc, &argv);
    scroll();
    terminal_shell_set();
}