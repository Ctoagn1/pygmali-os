#ifndef _KERNEL_CONSOLE_H
#define _KERNEL_CONSOLE_H
#include <stdint.h>
#include "keyboardhandler.h"
#include <stddef.h>
#include "multiboot.h"
#include "fd.h"
#define EXTRA_TEXT_BUFFER_SIZE  1024
extern const uintptr_t virtual_framebuffer;
struct File;
typedef struct{
	uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} Psf2_Header;
typedef struct{
	size_t terminal_row;
	size_t terminal_column;
	uint32_t fg;
	uint32_t bg;
} term_state_t;
extern term_state_t term;
extern uint32_t textbuffer_height;
extern uint32_t textbuffer_width;
extern Psf2_Header fontheader;
extern struct multiboot_tag_framebuffer* selected_video_mode;
extern uint32_t textbuffer_size;
extern const uint32_t default_background;
extern const uint32_t default_text;
extern uint64_t* terminal_buffer;


void terminal_initialize(struct multiboot_tag_framebuffer* fb);

void flush_buffer();
void screen_reset();
void print_os_name();
void terminal_putentryat(char c, size_t x, size_t y); 
void reload_buffer();
void text_to_pixels(uint32_t index);
unsigned char terminal_getcharat(size_t terminal_column, size_t terminal_row);
void terminal_putchar(char c);
void disable_cursor();
void update_cursor(int x, int y);
void enable_cursor(uint32_t cursor_start, uint32_t cursor_end);
void terminal_write(const char* data, size_t size); 
void terminal_writestring(const char* data);
void terminal_backspace();
void shift_forward_terminal_input();
void shift_backwards_terminal_input();
void scroll();
void terminal_set();
#endif
