#include "tty.h"
#include "pit.h"
#include "string.h"
#include "printf.h"
#include "inputhandler.h"
#include "rtc.h"
void cmd_echo(int argc, char argv[MAX_ARGS][ARG_SIZE]){
    for(int i=1; i<=argc; i++){
        terminal_writestring(argv[i]);
        terminal_putchar(' ');
    }
    terminal_putchar('\n');
}

void cmd_help(int argc, char argv[MAX_ARGS][ARG_SIZE]){
    terminal_writestring("Commands-\necho <what you want to repeat>, help, play <note, C1-B6, accidentals must be written as note-SHARP-octave, like ASHARP4> <duration in ms, cap is 10000\n");
}
void cmd_play_note(int argc, char argv[MAX_ARGS][ARG_SIZE]){
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
void cmd_clear(int argc, char argv[MAX_ARGS][ARG_SIZE]){
    terminal_initialize();
}
void cmd_time(int argc, char argv[MAX_ARGS][ARG_SIZE]){
    display_time();
}