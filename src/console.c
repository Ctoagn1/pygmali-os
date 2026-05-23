#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "string.h"
#include "console.h"
#include "vga.h"
#include "io.h"
#include "kmalloc.h"
#include "idt.h"
#include "keyboardhandler.h"
#include "font8x8_basic.h"
#include "font8x16.h"
#include "fatparser.h"
#include "multiboot.h"
#include "fd.h"
#define KERNEL_FONT_WIDTH 8
#define KERNEL_FONT_HEIGHT 16
#define KERNEL_FONT_CHARACTER_BYTES 16
uint8_t pixel_size;
_Bool cursor_on=true;
uint32_t textbuffer_size;
const uint32_t default_background=0x0;
const uint32_t default_text=0xFFFFFF;
extern uint32_t framebuffer_1;


term_state_t term;

uint32_t memory_size;
char* fontfile;


const uint32_t virtual_framebuffer = (uint32_t)1021<<22;
struct multiboot_tag_framebuffer* selected_video_mode;

int cursor_column_state=0;
int cursor_row_state=0;

uint32_t textbuffer_height;
uint32_t textbuffer_width;
uint64_t* terminal_buffer;




uint64_t extended_terminal_buffer[EXTRA_TEXT_BUFFER_SIZE];

void map_display(){
	uintptr_t* buff_addr = (&framebuffer_1);
	uint64_t pagecount = (selected_video_mode->common.framebuffer_height*selected_video_mode->common.framebuffer_width*selected_video_mode->common.framebuffer_bpp>>3)>>12;
	for(uint64_t i=0; i<pagecount; i++){
	buff_addr[i]=selected_video_mode->common.framebuffer_addr+(i<<12)+3;
	}
}

void terminal_initialize(struct multiboot_tag_framebuffer* vid_mode ){
	selected_video_mode = vid_mode;
	map_display();
	term.terminal_row = 0;
	term.terminal_column = 0;
	term.fg = 0xFFFFFF;
	term.bg = 0;
	memory_size=selected_video_mode->common.framebuffer_width*selected_video_mode->common.framebuffer_height*(selected_video_mode->common.framebuffer_bpp/8);
	textbuffer_height=selected_video_mode->common.framebuffer_height/KERNEL_FONT_HEIGHT;
	textbuffer_width=selected_video_mode->common.framebuffer_width/KERNEL_FONT_WIDTH;
	textbuffer_size = textbuffer_width*textbuffer_height;
	terminal_buffer = kmalloc(textbuffer_size*sizeof(uint64_t));
	memset(terminal_buffer, 0,textbuffer_size*sizeof(uint64_t));
	screen_reset();
}




void screen_reset(){
	for(uint32_t i=0; i<(selected_video_mode->common.framebuffer_width*selected_video_mode->common.framebuffer_height); i++){
		((uint32_t*)virtual_framebuffer)[i]=term.bg;
	}
}


void print_os_name(){
	terminal_writestring("\n                                       ___       ____  _____\n");
	terminal_writestring("    ____  __  ______ _____ ___  ____ _/ (_)     / __ \\/ ___/\n");
	terminal_writestring("   / __ \\/ / / / __ `/ __ `__ \\/ __ `/ / /_____/ / / /\\__ \\ \n");
	terminal_writestring("  / /_/ / /_/ / /_/ / / / / / / /_/ / / /_____/ /_/ /___/ / \n");
	terminal_writestring(" / .___/\\__, /\\__, /_/ /_/ /_/\\__,_/_/_/      \\____//____/  \n");
	terminal_writestring("/_/    /____//____/                                         \n");
	
}

void terminal_putentryat(char c, size_t x, size_t y) 
{
	uint32_t index = y * textbuffer_width + x;
	terminal_buffer[index] = c |(term.fg<<8)|((uint64_t)term.bg<<32);
	text_to_pixels(index);
}
void reload_buffer(){
	for(uint32_t i=0; i<textbuffer_size; i++){
		text_to_pixels(i);
	}
}

void text_to_pixels(uint32_t index){
	char text = terminal_buffer[index];
	uint32_t color  = terminal_buffer[index]>>8;
	uint32_t bg_color = terminal_buffer[index]>>32;
	uint32_t* memorybuffer=((uint32_t*)(virtual_framebuffer));
	uint32_t cols = textbuffer_width; 
    uint32_t cell_x = index % cols;
    uint32_t cell_y = index / cols;
    uint32_t pixel_x = cell_x * KERNEL_FONT_WIDTH;
    uint32_t pixel_y = cell_y * KERNEL_FONT_HEIGHT;
	uint32_t* memorydest = memorybuffer+pixel_y*selected_video_mode->common.framebuffer_width+pixel_x;
	unsigned char* bitmap = font8x16[(unsigned char)text];
	if(text=='\0'){
		for(int i=0; i<KERNEL_FONT_HEIGHT; i++){
			for(int j=0; j<KERNEL_FONT_WIDTH; j++){
				memorydest[j+i*selected_video_mode->common.framebuffer_width]=bg_color;
			}
		}
		return;
	}
	for(int i=0; i<KERNEL_FONT_HEIGHT; i++){
		for(int j=0; j<KERNEL_FONT_WIDTH; j++){
			if(bitmap[i * ((KERNEL_FONT_WIDTH + 7) / 8)+ (j/8)] & (0x80 >> (j%8))){
				memorydest[j+i*selected_video_mode->common.framebuffer_width]=color;
			}
			else{
				memorydest[j+i*selected_video_mode->common.framebuffer_width]=term.bg;
			}
		}
	}
}

unsigned char terminal_getcharat(size_t terminal_column, size_t terminal_row){
	return (terminal_buffer[terminal_row*textbuffer_width+terminal_column]);
}

void terminal_putchar(char c) 
{
	if (c == '\n'){
        term.terminal_column = 0;
        if (++term.terminal_row == textbuffer_height){
			scroll();
		}
		return;	
    }
    if(c<0x20){
		terminal_putchar('^');
		terminal_putchar(c+'@');
		return;
	}
	

	terminal_putentryat(c, term.terminal_column, term.terminal_row);
	if (++term.terminal_column >= textbuffer_width) {
		term.terminal_column = 0;
		if (++term.terminal_row >= textbuffer_height)
			scroll();
	}
}

void disable_cursor()
{
	cursor_on=false;
}
void update_cursor(int x, int y)
{
	uint32_t pos = y * textbuffer_width + x;
	(void) pos;
	return;

}
void enable_cursor(uint32_t cursor_start, uint32_t cursor_end)
{
	cursor_on=true;
	update_cursor(cursor_start, cursor_end);
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}
void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}


void terminal_backspace(){
	if(term.terminal_column==0 && term.terminal_row==(textbuffer_height-1)){
		memmove(&terminal_buffer[textbuffer_width], &terminal_buffer[0], textbuffer_width*(textbuffer_height-1)*sizeof(terminal_buffer[0])); //shift text down
		memmove(&terminal_buffer[0], &extended_terminal_buffer[0], textbuffer_width*sizeof(terminal_buffer[0])); // load text from buffer
		memmove(&extended_terminal_buffer[0], &extended_terminal_buffer[textbuffer_width], (EXTRA_TEXT_BUFFER_SIZE-textbuffer_width)*sizeof(terminal_buffer[0])); //shift buffer
		term.terminal_column = textbuffer_width-1;
		
	}
	else{
		term.terminal_column--;
	}
	terminal_putentryat(' ', term.terminal_column, term.terminal_row);
	update_cursor(term.terminal_column, term.terminal_row);
	return;
}

void shift_forward_terminal_input(){
	memmove(&terminal_buffer[term.terminal_row*textbuffer_width+term.terminal_column],&terminal_buffer[term.terminal_row*textbuffer_width+term.terminal_column-1], (textbuffer_width*textbuffer_height)-(term.terminal_row*textbuffer_width+term.terminal_column+1));
}
void shift_backwards_terminal_input(){
	memmove(&terminal_buffer[term.terminal_row*textbuffer_width+term.terminal_column],&terminal_buffer[term.terminal_row*textbuffer_width+term.terminal_column+1], (textbuffer_width*textbuffer_height)-(term.terminal_row*textbuffer_width+term.terminal_column+1));
}

void scroll(){
	term.terminal_column=0;
	term.terminal_row=textbuffer_height-1;
	reload_buffer();
	memmove(&extended_terminal_buffer[textbuffer_width], &extended_terminal_buffer[0], (EXTRA_TEXT_BUFFER_SIZE-textbuffer_width)*sizeof(extended_terminal_buffer[0])); //move buffer up
	memmove(&extended_terminal_buffer[0], &terminal_buffer[0], textbuffer_width * sizeof(terminal_buffer[0])); //move text into buffer
	memmove(&terminal_buffer[0], &terminal_buffer[textbuffer_width], textbuffer_width*(textbuffer_height-1)*sizeof(terminal_buffer[0])); //move text up
    for (size_t x = 0; x < textbuffer_width; x++) {
        terminal_buffer[(textbuffer_height-1)*textbuffer_width+x] = 0;
    } 
	for(uint8_t i=0; i<textbuffer_width; i++){
		terminal_putentryat(' ', i, term.terminal_row);
	}
	term.terminal_row=textbuffer_height-1;
	reload_buffer();
}
