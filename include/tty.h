#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H
#include <stdint.h>
#include <stddef.h>
#define EXTRA_TEXT_BUFFER_SIZE  1024
extern const uint32_t virtual_framebuffer;
typedef struct{
	uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;			// deprecated
	uint8_t window_b;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;			// height in pixels
	uint8_t w_char;			// unused...
	uint8_t y_char;			// ...
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;

	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;

	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} Video_Mode_Info;
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
extern Video_Mode_Info* selected_video_mode;

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
