#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H
#include <stdint.h>
#include <stddef.h>
#define EXTRA_TEXT_BUFFER_SIZE  1024
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
extern size_t terminal_row;
extern size_t terminal_column;
extern size_t input_start_column;
extern size_t input_start_row;
extern uint32_t terminal_color;
extern uint32_t textbuffer_height;
extern uint32_t textbuffer_width;
extern Psf2_Header fontheader;

extern _Bool is_input_from_user;
extern uint32_t* terminal_buffer;

void terminal_initialize(void);
void terminal_putchar(char c);
void psf_loader();
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void enable_cursor(uint32_t cursor_start, uint32_t cursor_end);
void update_cursor(int x, int y);
void disable_cursor();
unsigned char terminal_getcharat(size_t terminal_column, size_t terminal_row);
void print_os_name();
void reload_buffer();
void text_to_pixels(uint32_t index);
void video_startup();
void terminal_putentryat(char c, size_t x, size_t y);
void screen_reset();
int reload_fonts(char* filepath);
#endif
