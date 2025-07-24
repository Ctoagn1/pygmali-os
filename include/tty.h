#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
extern _Bool is_input_from_user;
void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void update_cursor(int x, int y);
void terminal_backspace();
unsigned char terminal_getcharat(size_t terminal_column, size_t terminal_row);
void terminal_shell_set();
void scroll();

#endif
