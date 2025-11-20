#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "string.h"
#include "tty.h"
#include "vga.h"
#include "tty.h"
#include "io.h"
#include "kmalloc.h"
#include "idt.h"
#include "keyboardhandler.h"
#include "inputhandler.h"
#include "writingmode.h"
#include "fatparser.h"
size_t terminal_row;
size_t terminal_column;
size_t input_start_column;
size_t input_start_row;
uint8_t pixel_size;
_Bool cursor_on=true;
Psf2_Header fontheader;
uint32_t textbuffer_size;
uint32_t backgroundcolor=0x0;
uint32_t memory_size;
char* fontfile;
const uint32_t virtual_framebuffer = (uint32_t)1021<<22;
Video_Mode_Info* selected_video_mode = (Video_Mode_Info*)0xc000f000;
uint32_t scroll_buffer[EXTRA_TEXT_BUFFER_SIZE];
_Bool is_input_from_user = 0;

uint32_t textbuffer_height;
uint32_t textbuffer_width;
uint32_t* terminal_buffer;
uint32_t terminal_color;

void terminal_initialize(void)
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = 0xFFFFFF;
	memory_size=selected_video_mode->width*selected_video_mode->height*(selected_video_mode->bpp/8);
}
void psf_loader(){
	char* loadfrom = file_contents("/SYS/FONT/FONTDATA");
	if(!loadfrom) panic("FONT NOT FOUND");
	fontfile = file_contents(loadfrom);
	kfree(loadfrom);
	fontheader = *(Psf2_Header*)fontfile;
	if(fontheader.magic!=0x864ab572) panic("INVALID FONT");
	textbuffer_height=selected_video_mode->height/fontheader.height;
	textbuffer_width=selected_video_mode->width/fontheader.width;
	textbuffer_size = textbuffer_width*textbuffer_height;
	terminal_buffer = kmalloc(textbuffer_size*sizeof(uint32_t));
	memset(terminal_buffer, 0,textbuffer_size*sizeof(uint32_t));
	screen_reset();
}
int reload_fonts(char* filepath){
	char* fontfile = file_contents(filepath);
	if(!fontfile) return 1;
	Psf2_Header header = *(Psf2_Header*)fontfile;
	if(header.magic!=0x864ab572){
		kfree(fontfile);
		return 1;
	}
	kfree(fontfile);
	write_to_file(filepath, strlen(filepath), "/SYS/FONT/FONTDATA");
	psf_loader();
	return 0;

}
void screen_reset(){
	for(int i=0; i<(selected_video_mode->width*selected_video_mode->height); i++){
		((uint32_t*)virtual_framebuffer)[i]=backgroundcolor;
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
	terminal_buffer[index] = c |(terminal_color<<8);
	text_to_pixels(index);
}
void reload_buffer(){
	for(int i=0; i<textbuffer_size; i++){
		text_to_pixels(i);
	}
}
void text_to_pixels(uint32_t index){
	char text = terminal_buffer[index];
	uint32_t color  = terminal_buffer[index]>>8;
	uint32_t* memorybuffer=((uint32_t*)(virtual_framebuffer));
	uint32_t cols = textbuffer_width; 
    uint32_t cell_x = index % cols;
    uint32_t cell_y = index / cols;
    uint32_t pixel_x = cell_x * fontheader.width;
    uint32_t pixel_y = cell_y * fontheader.height;
	uint32_t* memorydest = memorybuffer+pixel_y*selected_video_mode->width+pixel_x;
	char* bitmap = fontfile+sizeof(Psf2_Header)+text*fontheader.bytesperglyph;
	if(text=='\0'){
		for(int i=0; i<fontheader.height; i++){
			for(int j=0; j<fontheader.width; j++){
				memorydest[j+i*selected_video_mode->width]=backgroundcolor;
			}
		}
		return;
	}
	for(int i=0; i<fontheader.height; i++){
		for(int j=0; j<fontheader.width; j++){
			if(bitmap[i * ((fontheader.width + 7) / 8)+ (j/8)] & (0x80 >> (j%8))){
				memorydest[j+i*selected_video_mode->width]=color;
			}
			else{
				memorydest[j+i*selected_video_mode->width]=backgroundcolor;
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
        terminal_column = 0;
        if (++terminal_row == textbuffer_height){
			scroll();
		}
		return;	
    }
	if(c=='\t'){
		/*terminal_column+=4;
		if(terminal_column>= textbuffer_width){
			terminal_column%=textbuffer_width;
			terminal_row++;
			if(terminal_row==textbuffer_height){
				scroll();
			}
		}*/
		return;
	}

	terminal_putentryat(c, terminal_column, terminal_row);
	if (++terminal_column >= textbuffer_width) {
		terminal_column = 0;
		if (++terminal_row >= textbuffer_height)
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