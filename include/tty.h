#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000
#include <stdint.h>
#include <stddef.h>
#define EXTRA_TEXT_BUFFER_SIZE  1024
extern size_t terminal_row;
extern size_t terminal_column;
extern size_t input_start_column;
extern size_t input_start_row;
extern uint8_t terminal_color;
extern _Bool is_input_from_user;
extern uint16_t* terminal_buffer;

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void update_cursor(int x, int y);
void disable_cursor();
unsigned char terminal_getcharat(size_t terminal_column, size_t terminal_row);
void print_os_name();
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);

#endif
