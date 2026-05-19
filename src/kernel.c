#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "tty.h"
#include "pic.h"
#include "keyboardhandler.h"
#include "rtc.h"
#include "pit.h"
#include "printf.h"
#include "multiboot.h"
#include "process.h"
#include "kmalloc.h"
#include "diskreader.h"
#include "fatparser.h"
#include "paging.h"
struct multiboot_tag_framebuffer fb;
struct multiboot_tag_mmap *mmap;
void multiboot_iterate(struct multiboot_tag* tag){
	tag = tag+1; //skip 8 bytes
	while(1){
		if(tag->type==0) break;
		
		if(tag->type==8){
			 memcpy(&fb, tag, tag->size);
		}
		if(tag->type==6){
			mmap = (void*)tag;
		}
		tag += (tag->size+7)/8;
	}
}
void kernel_main(uint32_t magic, struct multiboot_tag* addr)
{
	multiboot_iterate(addr);
	initGdt();
	PIC_remap(0x20, 0x28);
	scan_mbr();
	paging_setup(mmap);
	initIdt();
	terminal_initialize(&fb);
	read_boot_record();
	/* Initialize terminal interface */
	set_hertz(1000);
	terminal_writestring("PygmaliOS is up and running!\n");
	tty_init();
	print_os_name();
	initialize_scheduling();
	read_startup_time();
	disable_translation();
	switch_scancode_set(2);
	display_time();
	unmask_timer();
}
