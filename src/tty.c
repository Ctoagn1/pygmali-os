#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "string.h"
#include "tty.h"
#include "vga.h"
#include "tty.h"
#include "io.h"
#include "keyboardhandler.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

size_t terminal_row;
size_t terminal_column;
size_t input_start_column;
size_t input_start_row;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;
_Bool is_input_from_user = 0;

void terminal_initialize(void)
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	if(is_input_from_user){
			terminal_shell_set();
		}
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry('\0', terminal_color);
		}
	}
}
void disable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}
void update_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
void terminal_shell_set(){
	terminal_column=0;
	terminal_row=VGA_HEIGHT-1;
	terminal_writestring("> ");
	input_start_column=terminal_column-1;
	input_start_row= terminal_row;
}
void terminal_putchar(char c) 
{
    if (c == '\n'){
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT){
			scroll();
		}
		if(is_input_from_user){
			terminal_shell_set();
		}
		return;	
    }
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			scroll();
	}
	update_cursor(terminal_column, terminal_row);
}
void terminal_backspace(){
	if(terminal_column==input_start_column+1 && terminal_row == input_start_row){
		return;
	}
	if(terminal_column == 0){ //this logic isn't really necessary for shell mode, but eh I already wrote it
		if(terminal_row == 0){
			return;
		}
		else{
			terminal_row--;
			terminal_column=VGA_WIDTH-1;
			while(terminal_getcharat(terminal_column, terminal_row) == 0 && terminal_column != 0)
				terminal_column--;
			if(terminal_getcharat(terminal_column, terminal_row) != 0)
				terminal_column++;
			update_cursor(terminal_column, terminal_row);
			return;
		}
	}
	terminal_column--;
	terminal_putchar('\0');
	terminal_column--;
	update_cursor(terminal_column, terminal_row);
	return;
}
unsigned char terminal_getcharat(size_t terminal_column, size_t terminal_row){
	return (terminal_buffer[terminal_row*VGA_WIDTH+terminal_column] & 0xFF);
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}
void scroll(){
    for (size_t y = 0; y < VGA_HEIGHT-1; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = terminal_buffer[index+VGA_WIDTH];
		}
	}
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT-1)*VGA_WIDTH+x] = vga_entry(' ', terminal_color);
    }
	terminal_row--;
	terminal_column=0;
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}