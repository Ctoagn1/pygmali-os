#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "string.h"
#include "console.h"
#include "vga.h"
#include "io.h"
#include "kmalloc.h"
#include "tty.h"
#include "idt.h"
#include "process.h"
#include "keyboardhandler.h"
#define STDIN_SIZE 4096*8
#define STDOUT_SIZE 4096*8
char Ascii_Lookup[256]={
    [KEY_A]='a', [KEY_B]='b', [KEY_C]='c', [KEY_D]='d', [KEY_E]='e', [KEY_F]='f', [KEY_G]='g', [KEY_H]='h', [KEY_I]='i', [KEY_J]='j', [KEY_K]='k',[KEY_L]='l',
    [KEY_M]='m', [KEY_N]='n', [KEY_O]='o', [KEY_P]='p', [KEY_Q]='q', [KEY_R]='r', [KEY_S]='s', [KEY_T]='t', [KEY_U]='u', [KEY_V]='v', [KEY_W]='w', [KEY_X]='x',
	[KEY_Y]='y', [KEY_Z]='z', [KEY_1]='1', [KEY_2]='2', [KEY_3]='3', [KEY_4]='4', [KEY_5]='5', [KEY_6]='6', [KEY_7]='7', [KEY_8]='8', [KEY_9]='9', [KEY_0]='0',
	[KEY_MINUS]='-', [KEY_EQUALS]='=', [KEY_OPEN_BRACKET]='[', [KEY_CLOSE_BRACKET]=']', [KEY_BACKSLASH]='\\', [KEY_BACK_TICK]='`', [KEY_SEMICOLON]=';', [KEY_APOSTROPHE]='\'',
	[KEY_COMMA]=',', [KEY_SPACE]=' ', [KEY_PERIOD]='.', [KEY_SLASH]='/'
};
char Shifted_Ascii_Lookup[256]={
    [KEY_A]='A', [KEY_B]='B', [KEY_C]='C', [KEY_D]='D', [KEY_E]='E', [KEY_F]='F', [KEY_G]='G', [KEY_H]='H', [KEY_I]='I', [KEY_J]='J', [KEY_K]='K',[KEY_L]='L',
    [KEY_M]='M', [KEY_N]='N', [KEY_O]='O', [KEY_P]='P', [KEY_Q]='Q', [KEY_R]='R', [KEY_S]='S', [KEY_T]='T', [KEY_U]='U', [KEY_V]='V', [KEY_W]='W', [KEY_X]='X',
	[KEY_Y]='Y', [KEY_Z]='Z', [KEY_1]='!', [KEY_2]='@', [KEY_3]='#', [KEY_4]='$', [KEY_5]='%', [KEY_6]='^', [KEY_7]='&', [KEY_8]='*', [KEY_9]='(', [KEY_0]=')',
	[KEY_MINUS]='_', [KEY_EQUALS]='+', [KEY_OPEN_BRACKET]='{', [KEY_CLOSE_BRACKET]='}', [KEY_BACKSLASH]='|', [KEY_BACK_TICK]='~', [KEY_SEMICOLON]=':', [KEY_APOSTROPHE]='\"',
	[KEY_COMMA]='<', [KEY_SPACE]=' ', [KEY_PERIOD]='>', [KEY_SLASH]='?'
};
char Ctrl_Lookup[256]={
    [KEY_SPACE]=0, [KEY_A]=1, [KEY_B]=2, [KEY_C]=3, [KEY_D]=4, [KEY_E]=5, [KEY_F]=6, [KEY_G]=7, [KEY_H]=8, [KEY_I]=9, [KEY_J]=10, [KEY_K]=11,[KEY_L]=12,
    [KEY_M]=13, [KEY_N]=14, [KEY_O]=15, [KEY_P]=16, [KEY_Q]=17, [KEY_R]=18, [KEY_S]=19, [KEY_T]=20, [KEY_U]=21, [KEY_V]=22, [KEY_W]=23, [KEY_X]=24,
	[KEY_Y]=25, [KEY_Z]=26, [KEY_1]=255, [KEY_2]=255, [KEY_3]=255, [KEY_4]=255, [KEY_5]=255, [KEY_6]=255, [KEY_7]=255, [KEY_8]=255, [KEY_9]=255, [KEY_0]=255,
	[KEY_MINUS]=25, [KEY_EQUALS]=255, [KEY_OPEN_BRACKET]=27, [KEY_CLOSE_BRACKET]=29, [KEY_BACKSLASH]=28, [KEY_BACK_TICK]=255, [KEY_SEMICOLON]=255, [KEY_APOSTROPHE]=255,
	[KEY_COMMA]=255, [KEY_PERIOD]=255, [KEY_SLASH]=255
};
uint32_t ansi_colors[16] = {
    0x000000, // black
    0xAA0000, // red
    0x00AA00, // green
    0xAA5500, // yellow
    0x0000AA, // blue
    0xAA00AA, // magenta
    0x00AAAA, // cyan
    0xAAAAAA, // white
    0x555555, // bright black / gray
    0xFF5555, // bright red
    0x55FF55, // bright green
    0xFFFF55, // bright yellow
    0x5555FF, // bright blue
    0xFF55FF, // bright magenta
    0x55FFFF, // bright cyan
    0xFFFFFF  // bright white
};
#define MAX_WAIT 10


typedef struct{
    char stdin_buffer[STDIN_SIZE];
    char stdout_buffer[STDOUT_SIZE];
    int stdin_read_index;
    int stdin_write_index;

    int stdout_read_index;
    int stdout_write_index;

    char input_buffer[INPUT_BUFFER_SIZE];
    int input_end;

    bool canonical;
    bool echo;

    Process* waiting_readers[MAX_WAIT];
    int waiting_count;

    term_state_t term;
} tty_t;
tty_t kernel_tty = {0};

void tty_init(){
    kernel_tty.canonical=true;
    kernel_tty.echo = true;
}

extern KeyEvent event_buffer[EVENT_BUFFER_SIZE];
int event_buffer_index=0;
void print_input(char newinput){
	if(kernel_tty.input_end==INPUT_BUFFER_SIZE-1) return;
    kernel_tty.input_buffer[kernel_tty.input_end] = newinput;
	kernel_tty.input_end++;
	if(kernel_tty.echo) terminal_putchar(newinput);
}
void del_input(){
	if(kernel_tty.input_end==0) return;
    kernel_tty.input_end-=1;
	char deleted_char=kernel_tty.input_buffer[kernel_tty.input_end];
	kernel_tty.input_buffer[kernel_tty.input_end]=0;
	if(kernel_tty.echo){
		if(deleted_char<0x20){
			terminal_backspace();
			terminal_backspace();
		}
		else terminal_backspace();
	} 
}

void clear_input_buffer(){
    memset(kernel_tty.input_buffer, 0, INPUT_BUFFER_SIZE);
	kernel_tty.input_end=0;
}


void stdout_parse(){
	char sequence_buffer[64]={0};
	while(kernel_tty.stdout_read_index!=kernel_tty.stdout_write_index){
		if(kernel_tty.stdout_buffer[kernel_tty.stdout_read_index]!='\e' || kernel_tty.stdout_buffer[(kernel_tty.stdout_read_index+1)%STDOUT_SIZE]!='['){
			terminal_putchar(kernel_tty.stdout_buffer[kernel_tty.stdout_read_index]);
			kernel_tty.stdout_read_index=(kernel_tty.stdout_read_index+1)%STDOUT_SIZE;
		}
		else{
			int size=0;
			char current_char = kernel_tty.stdout_buffer[kernel_tty.stdout_read_index];
			while((current_char<'a' || current_char>'z') && (current_char<'A' && current_char>'Z')){
				current_char = kernel_tty.stdout_buffer[(kernel_tty.stdout_read_index+size)%STDOUT_SIZE];
				sequence_buffer[size]=current_char;
				size++;
			}
			kernel_tty.stdout_read_index=(kernel_tty.stdout_read_index+size)%STDOUT_SIZE;
			parse_esc_sequence(&sequence_buffer[2]);

		}
	}
}

void parse_esc_sequence(char* seq) {
    int params[4] = {0};
    int param_count = 0;

    char* p = seq;
    int current = 0;

    while (*p) {
        if (*p >= '0' && *p <= '9') {
            current = current * 10 + (*p - '0');
        } else if (*p == ';') {
            params[param_count++] = current;
            current = 0;
        } else { 
            params[param_count++] = current;
            char command = *p;
            handle_sequence(command, params, param_count);
            break;
        }
        p++;
    }
}

void handle_sequence(char command, int params[4], int param_count){
	bool bold=false;
	switch(command){
		case 'A':
			if(term.terminal_row!=0)term.terminal_row--;
			break;
		case 'B':
			if(term.terminal_row!=(textbuffer_height-1)) term.terminal_row++;
			break;
		case 'C':
			if(term.terminal_column!=(textbuffer_width-1)) term.terminal_column++;
			break;
		case 'D':
			if(term.terminal_column!=0) term.terminal_column--;
			break;
		case 'm':
			for(int i=0; i<param_count; i++){
				if(params[i]==0){
					term.bg=default_background;
					term.fg=default_text;
				}
				if(params[i]==1){
					bold=true;
				}
				if(params[i]==2){
					//implement dim
				}
				if(params[i]==4){
					//implement underline
				}
				if(params[i]>=30 && params[i]<=37){
					term.fg = bold ? ansi_colors[params[i]-32] : ansi_colors[params[i]-40];
				}
				if(params[i]>=90 && params[i]<=97){
					term.fg=ansi_colors[params[i]-82];
				}
				if(params[i]>=40 && params[i]<=47){
					term.bg = bold ? ansi_colors[params[i]-32] : ansi_colors[params[i]-40];
				}
				if(params[i]>=100 && params[i]<=107){
					term.bg=ansi_colors[params[i]-92];
				}
			}
			break;
		case 'J':
			uint64_t blank = (uint64_t)term.bg<<32|term.fg<<8|0x20;
			if(params[0]==2){
				for(int i=0; i<textbuffer_size; i++){
					terminal_buffer[i]=blank;
				}
			}
			if(params[0]==0){
				int index = term.terminal_row*textbuffer_width+term.terminal_column;
				for(int i=index; i<textbuffer_size-index; i++){
					terminal_buffer[i]=blank;
				}
			}
			if(params[0]==1){
				int index = term.terminal_row*textbuffer_width+term.terminal_column;
				for(int i=textbuffer_size-1; i>=index; i--){
					terminal_buffer[i]=blank;
				}
			}
			break;
		case 'H':
			term.terminal_row=params[0]%textbuffer_height;
			term.terminal_column=params[1]%textbuffer_width;
	}
}

void keyevent_translate(){
	KeyEvent* new_event = &event_buffer[event_buffer_index];
	if(!new_event->new) return;
	new_event->new=false;
	event_buffer_index=(event_buffer_index+1)%EVENT_BUFFER_SIZE;
	if(!new_event->pressed) return;
    char c=0; 
	if(new_event->ctrl){
		if(!(new_event->keycode>=KEY_A && new_event->keycode<=KEY_SLASH || new_event->keycode==KEY_SPACE)) return;
		char val = Ctrl_Lookup[new_event->keycode];
		if(val==255) return; //signals invalid ctrl char
    }
	else if(((new_event->keycode>=KEY_A && new_event->keycode<=KEY_Z) || new_event->keycode==KEY_SPACE) && new_event->pressed){
		if(new_event->shift^new_event->capslock){
			c = Shifted_Ascii_Lookup[new_event->keycode];
		}
		else c=Ascii_Lookup[new_event->keycode];
	}
	else if(new_event->keycode>=KEY_BACK_TICK&&new_event->keycode<=KEY_SLASH){
		if(new_event->shift) c = Shifted_Ascii_Lookup[new_event->keycode];
		else  c =(Ascii_Lookup[new_event->keycode]);
	}
	else if(new_event->keycode==KEY_ENTER){
		c = '\n';
	}
	else if(new_event->keycode==KEY_BACKSPACE){
		c = '\b';
	}
    else return;
    if(new_event->alt) tty_receive_char('\e');
    tty_receive_char(c);
	
}
void wake_readers(){
    while(kernel_tty.waiting_count>0){
        kernel_tty.waiting_count--;
        wake_process(kernel_tty.waiting_readers[kernel_tty.waiting_count], BLOCK_WAIT_INPUT);

    }
}
void tty_receive_char(char c){
    if(kernel_tty.canonical){
        if(c=='\b'){
            del_input();
            return;
        }
        print_input(c);
        if(c=='\n'){
            flush_buffer();
        }
    }
    else{
        if(kernel_tty.echo) terminal_putchar(c);
        kernel_tty.stdin_buffer[kernel_tty.stdin_write_index]=c;
        kernel_tty.stdin_write_index = (kernel_tty.stdin_write_index + 1) % STDIN_SIZE;
    }
}

void flush_buffer(){
	for(int i=0; i<kernel_tty.input_end; i++){
		kernel_tty.stdin_buffer[(kernel_tty.stdin_write_index+i)%STDIN_SIZE]=kernel_tty.input_buffer[i];
	}
	kernel_tty.stdin_write_index=(kernel_tty.stdin_write_index+kernel_tty.input_end)%STDIN_SIZE;
	kernel_tty.input_end=0;
}

int stdin_read(File* f, void* buf, int n){
	char *bytes = buf;
	int bytes_read = 0;
	while (bytes_read < n) {
        while(kernel_tty.stdin_read_index == kernel_tty.stdin_write_index) {
            current_process->p.state = PROC_BLOCKED;
            current_process->p.block_reason = BLOCK_WAIT_INPUT;
            kernel_tty.waiting_readers[kernel_tty.waiting_count]=&current_process->p;
            if(kernel_tty.waiting_count<MAX_WAIT-1) kernel_tty.waiting_count++;
        }
        char c = kernel_tty.stdin_buffer[kernel_tty.stdin_read_index];
        bytes[bytes_read]=c;
        kernel_tty.stdin_read_index = (kernel_tty.stdin_read_index + 1) % STDIN_SIZE;
        bytes_read++;
        if (c=='\n' && kernel_tty.canonical) break;
    }
    return bytes_read;
}


int stdout_write(File* f, const void* buf, int n){
	const char *bytes = buf;
	for(int i=0; i<n; i++){
		kernel_tty.stdout_buffer[(kernel_tty.stdout_write_index+i)%STDOUT_SIZE]=bytes[i];
	}
	kernel_tty.stdout_write_index=(kernel_tty.stdout_write_index+n)%STDOUT_SIZE;
	return n;
}

