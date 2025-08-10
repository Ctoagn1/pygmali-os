#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000
#include "tty.h"
#include "string.h"
#include "vga.h"
#include "keyboardhandler.h"
#include "writingmode.h"
#include "kmalloc.h"
#include "galatea.h"
#include "inputhandler.h"
int mode=1;
uint16_t shell_buffer[EXTRA_TEXT_BUFFER_SIZE];
char input_buffer[INPUT_BUFFER_SIZE] = {0};
size_t input_len = 0;
int input_pos = 0;
void add_to_input_buffer(char newinput){
    if(input_len<INPUT_BUFFER_SIZE-1){
		memmove(&input_buffer[input_pos+1], &input_buffer[input_pos],input_len-input_pos+1);
        input_buffer[input_pos] = newinput;
        input_len++;
		input_pos++;
    }
}
void remove_from_input_buffer(){
    if(input_len>0){
        input_len--;
		input_pos--;
		memmove(&input_buffer[input_pos-1], &input_buffer[input_pos], input_len-input_pos);
        input_buffer[input_len]='\0';
    }
}

void clear_input_buffer(){
    memset(input_buffer, 0, INPUT_BUFFER_SIZE);
    input_len=0;
	input_pos=0;
}
void shell_backspace(){
	if(terminal_row == input_start_row && terminal_column == input_start_column+1){
		return;
	}
	if(terminal_column==0 && terminal_row==(VGA_HEIGHT-1)){
		memmove(&terminal_buffer[VGA_WIDTH], &terminal_buffer[0], VGA_WIDTH*(VGA_HEIGHT-1)*sizeof(terminal_buffer[0])); //shift text down
		memmove(&terminal_buffer[0], &shell_buffer[0], VGA_WIDTH*sizeof(terminal_buffer[0])); // load text from buffer
		memmove(&shell_buffer[0], &shell_buffer[VGA_WIDTH], (EXTRA_TEXT_BUFFER_SIZE-VGA_WIDTH)*sizeof(terminal_buffer[0])); //shift buffer
		terminal_column = VGA_WIDTH-1;
		input_start_row++;
		
	}
	else{
		terminal_column--;
	}
	terminal_putentryat('\0', terminal_color, terminal_column, terminal_row);
	update_cursor(terminal_column, terminal_row);
	return;
}
void clear_beyond_input(){
	memset(&terminal_buffer[input_start_row*VGA_WIDTH+input_start_column], 0, (VGA_WIDTH*VGA_HEIGHT)-(input_start_row*VGA_WIDTH+input_start_column));
	terminal_row=input_start_row;
	terminal_column=input_start_column+1;
}
void shift_forward_shell_input(){
	memmove(&terminal_buffer[terminal_row*VGA_WIDTH+terminal_column],&terminal_buffer[terminal_row*VGA_WIDTH+terminal_column-1], (VGA_WIDTH*VGA_HEIGHT)-(terminal_row*VGA_WIDTH+terminal_column+1));
}
void shift_backwards_shell_input(){
	memmove(&terminal_buffer[terminal_row*VGA_WIDTH+terminal_column],&terminal_buffer[terminal_row*VGA_WIDTH+terminal_column+1], (VGA_WIDTH*VGA_HEIGHT)-(terminal_row*VGA_WIDTH+terminal_column+1));
}
void shell_scroll(){
	if(input_start_row==0 && is_input_from_user){
		terminal_column=0;
		terminal_row=VGA_HEIGHT-1;
		for(uint8_t i=0; i<VGA_WIDTH; i++){
			terminal_putentryat('\0', terminal_color, i, terminal_row);
		}
		return;
	}
	memmove(&shell_buffer[VGA_WIDTH], &shell_buffer[0],(EXTRA_TEXT_BUFFER_SIZE-VGA_WIDTH)*sizeof(shell_buffer[0])); //move buffer up
	memmove(&shell_buffer[0], &terminal_buffer[0], VGA_WIDTH * sizeof(terminal_buffer[0])); //move text into buffer
	memmove(&terminal_buffer[0], &terminal_buffer[VGA_WIDTH], VGA_WIDTH*(VGA_HEIGHT-1)*sizeof(terminal_buffer[0])); //move text up
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT-1)*VGA_WIDTH+x] = vga_entry('\0', terminal_color);
    } 
	input_start_row--;
	terminal_row=VGA_HEIGHT-1;
}
void terminal_shell_set(){
	terminal_column=0;
	terminal_row=VGA_HEIGHT-1;
	terminal_writestring("> ");
	input_start_column=terminal_column-1;
	input_start_row= terminal_row;
}
void terminal_backspace(){
    shell_backspace();
}
void scroll(){
    shell_scroll();
}
void keyparse(KeyEvent key){
	if(mode==1){
		if(key.scancode==BACKSPACE_KEY){
			shell_backspace();
			remove_from_input_buffer();
			shift_backwards_shell_input();
			return;
		}
		if(key.scancode==CURSOR_UP && key.special==1){
			clear_beyond_input();
			back_history();
		}
		if(key.scancode==CURSOR_DOWN && key.special==1){
			clear_beyond_input();
			forward_history();
		}
		if(key.scancode==CURSOR_LEFT && key.special==1){
			if(terminal_row == input_start_row && terminal_column == input_start_column+1){
				return;
			}
			input_pos--;
			terminal_column--;
			if(terminal_column<0){
				terminal_column=VGA_WIDTH-1;
				terminal_row--;
			}
			update_cursor(terminal_column, terminal_row);
		}
		if(key.scancode==CURSOR_RIGHT && key.special==1){
			if(terminal_column+1==VGA_WIDTH){
				if(terminal_getcharat(0, terminal_row+1)=='\0' && terminal_getcharat(terminal_column, terminal_row)=='\0'){
					return;
				}
			}
			else{
				if(terminal_getcharat(terminal_column+1, terminal_row)=='\0' && terminal_getcharat(terminal_column, terminal_row)=='\0'){
					return;
				}
			}
			input_pos++;
			terminal_column++;
			if(terminal_column==VGA_WIDTH){
				terminal_column=0;
				terminal_row++;
			}
			update_cursor(terminal_column, terminal_row);
		}
		if(key.ascii != '\0'){
			 shell_print(key.ascii);
		}
	}
	if(mode==2){
		editor_parse(key);
	}
}
void shell_print(char c){
    if (c == '\n'){
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT){
			shell_scroll();
			parse_and_run();
		}
		return;	
    }
	if(c=='\t') return;
	shift_forward_shell_input();
	terminal_putchar(c);
	add_to_input_buffer(c);
	update_cursor(terminal_column, terminal_row);
}
void screen_writer(){
    KeyEvent key = {0};
    if(get_keyevent(&key)) keyparse(key);
}