
#include <stdint.h>
#include "keyboardhandler.h"
#include "io.h"
#include "tty.h"
#include "pic.h"
_Bool key_state[KEYBOARD_SIZE] = {0};
char ascii_map[ASCII_MAP_SIZE] = {[A_KEY]='a',[B_KEY]='b',[C_KEY]='c',[D_KEY]='d',[E_KEY]='e',
[F_KEY]='f',[G_KEY]='g',[H_KEY]='h',[I_KEY]='i',[J_KEY]='j',[K_KEY]='k',[L_KEY]='l',[M_KEY]='m',
[N_KEY]='n',[O_KEY]='o',[P_KEY]='p',[Q_KEY]='q',[R_KEY]='r',[S_KEY]='s',[T_KEY]='t',[U_KEY]='u',
[V_KEY]='v',[W_KEY]='w',[X_KEY]='x',[Y_KEY]='y',[Z_KEY]='z',[COMMA_KEY]=',',[PERIOD_KEY]='.',[SLASH_KEY]='/',
[APOSTROPHE_KEY]='\'',[SEMICOLON_KEY]=';',[OPEN_BRACKET_KEY]='[',[CLOSED_BRACKET_KEY]=']',[BACKSLASH_KEY]='\\',[BACK_TICK]='`',
[_0_KEY]='0',[_1_KEY]='1',[_2_KEY]='2',[_3_KEY]='3',[_4_KEY]='4',[_5_KEY]='5',[_6_KEY]='6',[_7_KEY]='7',[_8_KEY]='8',
[_9_KEY]='9',[MINUS_KEY]='-',[EQUALS_KEY]='=', [ENTER_KEY]='\n', [TAB_KEY]='\t',[SPACE_KEY]= ' '};
char shift_ascii_map[ASCII_MAP_SIZE] = {[A_KEY]='A',[B_KEY]='B',[C_KEY]='C',[D_KEY]='D',[E_KEY]='E',
[F_KEY]='F',[G_KEY]='G',[H_KEY]='H',[I_KEY]='I',[J_KEY]='J',[K_KEY]='K',[L_KEY]='L',[M_KEY]='M',
[N_KEY]='N',[O_KEY]='O',[P_KEY]='P',[Q_KEY]='Q',[R_KEY]='R',[S_KEY]='S',[T_KEY]='T',[U_KEY]='U',
[V_KEY]='V',[W_KEY]='W',[X_KEY]='X',[Y_KEY]='Y',[Z_KEY]='Z',[COMMA_KEY]='<',[PERIOD_KEY]='>',[SLASH_KEY]='?',
[APOSTROPHE_KEY]='\"',[SEMICOLON_KEY]=':',[OPEN_BRACKET_KEY]='{',[CLOSED_BRACKET_KEY]='}',[BACKSLASH_KEY]='|',[BACK_TICK]='~',
[_0_KEY]=')',[_1_KEY]='!',[_2_KEY]='@',[_3_KEY]='#',[_4_KEY]='$',[_5_KEY]='%',[_6_KEY]='^',[_7_KEY]='&',[_8_KEY]='*',
[_9_KEY]='(',[MINUS_KEY]='_',[EQUALS_KEY]='+', [ENTER_KEY]='\n', [TAB_KEY]='\t',[SPACE_KEY]= ' '};


static uint8_t keyboard_buffer[BUFFER_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;


void write_to_buffer(){
    uint8_t next = (head + 1) % BUFFER_SIZE;
    if (next == tail){ //checks if buffer is full
        return;
    }
    keyboard_buffer[head] = inb(KEYBOARD_DATA);
    head = next;
    PIC_sendEOI(1);
}
uint8_t switch_scancode_set(uint8_t set){
    int timeout = 10000;
    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("switch_scancode_set timed out.\n");
            return 0xFF;
        } 
    }
    timeout = 10000;
    outb(KEYBOARD_DATA,0xF0);
    while((inb(KEYBOARD_COMMAND) & 1)){ /*
        really you should wait for the output buffer on the lsb to be full instead of empty but
        (inb(KEYBOARD_COMMAND)&1==0) doesn't work while this does for some reason so idk
        TODO- figure out why waiting for output to be full before reading doesn't work*/
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("switch_scancode_set timed out.\n");
            return 0xFF;
        } 
    }
    timeout = 10000; //lsb is output buffer
    if (inb(KEYBOARD_DATA) != 0xFA) return 0xFF;

    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("switch_scancode_set timed out.\n");
            return 0xFF;
        } 
    }
    timeout = 10000;//2nd lsb is input buffer, wait until 0 so it can take new input
    outb(KEYBOARD_DATA, set);

    while((inb(KEYBOARD_COMMAND) & 1)){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("switch_scancode_set timed out.\n");
            return 0xFF;
        } 
    }
    if(inb(KEYBOARD_DATA) != 0xFA){
        return 0xFF;
    }
    terminal_writestring("switch_scancode_set ran successfully...\n");
    return 0;
} //value of 0 gets current scancode set, 1 is scan code set , 2 for set 2, 3 for set 3. 2 is universal support


void disable_translation(){
    int timeout=10000;
    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("disable_translation timed out.\n");
            return;
        } 
    }
    timeout = 10000;
    outb(KEYBOARD_COMMAND,0x20); //read command byte command

    while((inb(KEYBOARD_COMMAND) & 1)){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("disable_translation timed out.\n");
            return;
        } 
    }
    uint8_t command_byte = inb(KEYBOARD_DATA);
    command_byte &= 0b10111111; //disables translation to scancode set 1
    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("disable_translation timed out.\n");
            return;
        } 
    }
    timeout = 10000;
    outb(KEYBOARD_COMMAND, 0x60); //write command byte command

    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("disable_translation timed out.\n");
            return;
        } 
    }
    outb(KEYBOARD_DATA, command_byte); //writes updated command byte
    terminal_writestring("disable_translation ran successfully...\n");
}


_Bool read_from_buffer(uint16_t *data){ //0 on success, 1 on failure
    if(head == tail){
        return 1;
    }
    uint16_t value = 0;
    if (keyboard_buffer[tail] == 0xE0){ //all scancodes that start with 0xE0 are at least 2 bytes
        if(keyboard_buffer[(tail+1)%BUFFER_SIZE] == 0xF0){
            if(keyboard_buffer[(tail+2)%BUFFER_SIZE] == 0x7C){
                value = PRINTSCREEN_RELEASE; //prntscrn released
                tail = (tail+6)%BUFFER_SIZE;
                *data = value;
                return 0;
            }
            value = keyboard_buffer[(tail+2)%BUFFER_SIZE]+E0F0_OFFSET; //0xF0 scancodes end at 387, 0xE0 0xF0 go from 387 to 512
            tail = (tail+3)%BUFFER_SIZE;
            *data = value;
            return 0;
        }
        if(keyboard_buffer[tail+1] == 0x12){
            value = PRINTSCREEN_PRESS; //prntscrn pressed
            tail = (tail+4)%BUFFER_SIZE;
            *data = value;
            return 0;
        }
        value = keyboard_buffer[(tail+1)%BUFFER_SIZE] +E0_OFFSET; //regular scancodes end at 131, 0xE0 go from 132-256
        tail = (tail+2)%BUFFER_SIZE;
        *data = value;
        return 0;
    }
    if(keyboard_buffer[tail] == 0xE1){
        value = PAUSE; //pause pressed
        tail = (tail+8)%BUFFER_SIZE;
        *data = value;
        return 0;
    }
    if(keyboard_buffer[tail] == 0xF0){
        value = keyboard_buffer[(tail+1)%BUFFER_SIZE]+F0_OFFSET; //0xE0 scancodes end at 256, 0xF0 go from 257-387
        tail = (tail+2)%BUFFER_SIZE;
        *data = value;
        return 0;
    }
    value = keyboard_buffer[tail];
    tail = (tail+1)%BUFFER_SIZE;
    *data = value;
    return 0;
}
void update_key_state(uint16_t data){

    if(data > 256 && data < 513){ //release codes should be exactly 256 above their press down counterparts
        if(data == F0_OFFSET+CAPSLOCK_KEY){
            key_state[CAPSLOCK_KEY] = !key_state[CAPSLOCK_KEY];
        }
        key_state[data-256]=0;
    }
    else{
        key_state[data]=1; //don't really care about prntscrn or pause right now
    }
    return;
}
char scancode_to_char(uint16_t keynum){
    if(!key_state[keynum] || keynum >= ASCII_MAP_SIZE){
        return '\0';
    }
    char keyletter;
    _Bool shiftOn = key_state[LEFT_SHIFT] || key_state[RIGHT_SHIFT_KEY];
    if (shiftOn ^ key_state[CAPSLOCK_KEY]){
        keyletter = shift_ascii_map[keynum];
        return keyletter;
    }
    keyletter = ascii_map[keynum];
    return keyletter;
    
}
void screen_writer(){
    is_input_from_user=1;
    uint16_t data;
    if(read_from_buffer(&data)){
        return;
    }
    if(data == BACKSPACE_KEY){
        terminal_backspace();
        return;
    }
    update_key_state(data);
    char key = scancode_to_char(data);
    if(key != '\0'){
        terminal_putchar(key);
    }
}