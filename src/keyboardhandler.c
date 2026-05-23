
#include <stdint.h>
#include "keyboardhandler.h"
#include "io.h"
#include "tty.h"
#include "console.h"
#include <stdint.h>
#include "pic.h"
#include <stdbool.h>
#include "string.h"
bool key_state[512] = {0};


char Regular_Key_Lookup[]={
    [0x01]=KEY_F9, [0x03]=KEY_F5, [0x04]=KEY_F3, [0x05]=KEY_F1, [0x06]=KEY_F2, [0x07]=KEY_F12,
    [0x09]=KEY_F10, [0x0A]=KEY_F8, [0x0B]=KEY_F6, [0x0C]=KEY_F4, [0x0D]=KEY_TAB, [0x0E]=KEY_BACK_TICK,
    [0x11]=KEY_LEFT_ALT, [0x12]=KEY_LEFT_SHIFT, [0x14]=KEY_LEFT_CTRL, [0x15]=KEY_Q, [0x16]=KEY_1, [0x1A]=KEY_Z,
    [0x1B]=KEY_S, [0x1C]=KEY_A, [0x1D]=KEY_W, [0x1E]=KEY_2, [0x21]=KEY_C, [0x22]=KEY_X, [0x23]=KEY_D, [0x24]=KEY_E,
    [0x25]=KEY_4, [0x26]=KEY_3, [0x29]=KEY_SPACE, [0x2A]=KEY_V, [0x2B]=KEY_F, [0x2C]=KEY_T, [0x2D]=KEY_R, [0x2E]=KEY_5,
    [0x31]=KEY_N, [0x32]=KEY_B, [0x33]=KEY_H, [0x34]=KEY_G, [0x35]=KEY_Y, [0x36]=KEY_6, [0x3A]=KEY_M, [0x3B]=KEY_J, [0x3C]=KEY_U,
    [0x3D]=KEY_7, [0x3E]=KEY_8, [0x41]=KEY_COMMA, [0x42]=KEY_K, [0x43]=KEY_I, [0x44]=KEY_O, [0x45]=KEY_0, [0x46]=KEY_9, [0x49]=KEY_PERIOD,
    [0x4A]=KEY_SLASH, [0x4B]=KEY_L, [0x4C]=KEY_SEMICOLON, [0x4D]=KEY_P, [0x4E]=KEY_MINUS, [0x52]=KEY_APOSTROPHE, [0x54]=KEY_OPEN_BRACKET,
    [0x55]=KEY_EQUALS, [0x58]=KEY_CAPSLOCK, [0x59]=KEY_RIGHT_SHIFT, [0x5A]=KEY_ENTER, [0x5B]=KEY_CLOSE_BRACKET, [0x5D]=KEY_BACKSLASH,
    [0x66]=KEY_BACKSPACE, [0x69]=KEYPAD_1, [0x6B]=KEYPAD_4, [0x6C]=KEYPAD_7, [0x70]=KEYPAD_0, [0x71]=KEYPAD_DEC, [0x72]=KEYPAD_2, [0x73]=KEYPAD_5,
    [0x74]=KEYPAD_6, [0x75]=KEYPAD_8, [0x76]=KEY_ESC, [0x77]=KEY_NUMBER_LOCK, [0x78]=KEY_F11, [0x79]=KEYPAD_PLUS, [0x7A]=KEYPAD_3, [0x7B]=KEYPAD_MINUS,
    [0x7C]=KEYPAD_MUL, [0x7D]=KEYPAD_9, [0x7E]=KEY_SCROLL_LOCK, [0x83]=KEY_F7

};
char E0_Lookup[]={
    [0x10]=KEY_WWW_SEARCH, [0x11]=KEY_RIGHT_ALT, [0x14]=KEY_RIGHT_CTRL, [0x15]=KEY_PREV_TRACK, [0x18]=KEY_WWW_FAVORITES,
    [0x1F]=KEY_LEFT_GUI, [0x20]=KEY_WWW_STOP, [0x21]=KEY_VOL_DOWN, [0x23]=KEY_MUTE, [0x27]=KEY_RIGHT_GUI, [0x28]=KEY_WWW_STOP, [0x2B]=KEY_CALCULATOR,
    [0x2f]=KEY_APPS, [0x30]=KEY_WWW_FORWARD, [0x32]=KEY_VOL_UP, [0x34]=KEY_PLAY_PAUSE, [0x37]=KEY_ACPI_POWER, [0x38]=KEY_WWW_BACK,
    [0x3a]=KEY_HOME, [0x3b]=KEY_STOP, [0x3f]=KEY_ACPI_SLEEP, [0x40]=KEY_MY_COMPUTER, [0x48]=KEY_EMAIL, [0x4A]=KEYPAD_DIV, [0x4d]=KEY_NEXT_TRACK,
    [0x50]=KEY_MEDIA_SELECT, [0x5a]=KEY_ENTER, [0x5e]=KEY_ACPI_WAKE, [0x69]=KEY_END, [0x6B]=CURSOR_LEFT, [0x6C]=KEY_HOME, [0x70]=KEY_INSERT,
    [0x71]=KEY_DELETE, [0x72]=CURSOR_DOWN, [0x74]=CURSOR_RIGHT, [0x75]=CURSOR_UP, [0x7A]=KEY_PGDN, [0x7D]=KEY_PGUP
};

KeyEvent event_buffer[EVENT_BUFFER_SIZE];
int event_buffer_offset=0;
static uint8_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;
int code_size=0;

void write_to_buffer(){
    int next = (head + 1) % KEYBOARD_BUFFER_SIZE;
    bool pause;
    if (next == tail){ //checks if buffer is full
        return;
    }
    keyboard_buffer[head] = inb(KEYBOARD_DATA);
    uint8_t scancode = keyboard_buffer[head];
    if(scancode==0xe1) pause=true;
    head = next;

    code_size++;
    if(!(scancode==0xE0 || scancode==0xF0 || (scancode==0x7C && code_size==3) || (pause&&code_size!=8))){
        read_from_buffer();
        code_size=0;
        pause=false;
    }
    PIC_sendEOI(1);
}
uint8_t switch_scancode_set(uint8_t set){
    int timeout = 100000;
    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("switch_scancode_set timed out.\n");
            return 0xFF;
        } 
    }
    timeout = 100000;
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
    timeout = 100000; //lsb is output buffer
    if (inb(KEYBOARD_DATA) != 0xFA) return 0xFF;

    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("switch_scancode_set timed out.\n");
            return 0xFF;
        } 
    }
    timeout = 100000;//2nd lsb is input buffer, wait until 0 so it can take new input
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
    terminal_writestring("Keyboard set up successfully...\n");
    return 0;
} //value of 0 gets current scancode set, 1 is scan code set , 2 for set 2, 3 for set 3. 2 is universal support


void disable_translation(){
    int timeout=100000;
    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("disable_translation timed out.\n");
            return;
        } 
    }
    timeout = 100000;
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
    timeout = 100000;
    outb(KEYBOARD_COMMAND, 0x60); //write command byte command

    while (inb(KEYBOARD_COMMAND) & 2){
        io_wait();
        timeout--;
        if (!timeout){
            terminal_writestring("disable_translation timed out.\n");
            return;
        } 
    }
    outb(KEYBOARD_DATA, command_byte); //writes updated command byte;
}


void read_from_buffer(){
    if(head == tail){
        return;
    }
    uint8_t val = 0;
    if (keyboard_buffer[tail] == 0xE0){ //all scancodes that start with 0xE0 are at least 2 bytes
        if(keyboard_buffer[(tail+1)%KEYBOARD_BUFFER_SIZE] == 0xF0){
            if(keyboard_buffer[(tail+2)%KEYBOARD_BUFFER_SIZE] == 0x7C){
                key_state[KEY_PRNTSCRN]=0;
                tail = (tail+6)%KEYBOARD_BUFFER_SIZE;
                push_keyevent(KEY_PRNTSCRN, false);
                return;
            }
            val = keyboard_buffer[(tail+2)%KEYBOARD_BUFFER_SIZE];
            key_state[(unsigned char)E0_Lookup[val]]=0;
            tail = (tail+3)%KEYBOARD_BUFFER_SIZE;
            push_keyevent(E0_Lookup[val], false);
            return;
        }
        if(keyboard_buffer[(tail+1)%KEYBOARD_BUFFER_SIZE] == 0x12){
            key_state[KEY_PRNTSCRN]=1;
            tail = (tail+4)%KEYBOARD_BUFFER_SIZE;
            push_keyevent(KEY_PRNTSCRN, true);
            return;
        }
        val = keyboard_buffer[(tail+1)%KEYBOARD_BUFFER_SIZE];
        key_state[(unsigned char)E0_Lookup[val]]=1;
        tail = (tail+2)%KEYBOARD_BUFFER_SIZE;
        push_keyevent(E0_Lookup[val], true);
        return;
    }
    if(keyboard_buffer[tail] == 0xE1){
        tail = (tail+8)%KEYBOARD_BUFFER_SIZE; //pause acts as if instantly released
        push_keyevent(KEY_PAUSE, true);
        return;
    }
    if(keyboard_buffer[tail] == 0xF0){
        val = keyboard_buffer[(tail+1)%KEYBOARD_BUFFER_SIZE];
        if(val!=KEY_CAPSLOCK && val!=KEY_NUMBER_LOCK && val!=KEY_SCROLL_LOCK) key_state[(unsigned char)Regular_Key_Lookup[val]]=0;
        tail = (tail+2)%KEYBOARD_BUFFER_SIZE;
        push_keyevent(Regular_Key_Lookup[val], false);
        return;
    }
    val = keyboard_buffer[tail];
    if(val!=KEY_CAPSLOCK && val!=KEY_NUMBER_LOCK && val!=KEY_SCROLL_LOCK) key_state[(unsigned char)Regular_Key_Lookup[val]]=1;
    else key_state[(unsigned char)Regular_Key_Lookup[val]]=!key_state[(unsigned char)Regular_Key_Lookup[val]];
    tail = (tail+1)%KEYBOARD_BUFFER_SIZE;
    push_keyevent(Regular_Key_Lookup[val], true);
    return;
}


void push_keyevent(Keycode code, bool pressed){
    KeyEvent newevent;
    newevent.alt=(key_state[KEY_LEFT_ALT]||key_state[KEY_RIGHT_ALT]);
    newevent.pressed = pressed;
    newevent.capslock = (key_state[KEY_CAPSLOCK]);
    newevent.ctrl = (key_state[KEY_LEFT_CTRL]||key_state[KEY_RIGHT_CTRL]);
    newevent.shift = (key_state[KEY_LEFT_SHIFT]||key_state[KEY_RIGHT_SHIFT]);
    newevent.numlock = key_state[KEY_NUMBER_LOCK];
    newevent.keycode = code;
    newevent.new=true;
    event_buffer[event_buffer_offset]=newevent;
    event_buffer_offset=(event_buffer_offset+1)%EVENT_BUFFER_SIZE;
}