#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tty.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboardhandler.h"
#include "rtc.h"
#include "pit.h"
#include "printf.h"
#include "kmalloc.h"
#include "diskreader.h"


void kernel_main(unsigned long magic, unsigned long mb_info_ptr)
{
	if(magic == 0x2BADB002){ //signifies multiboot
		heap_end = (void*)mb_info_ptr;
	}
	heap_start= (void*)ALIGN16((uint64_t) &_end);
	/* Initialize terminal interface */
	set_hertz(1000);
	terminal_initialize();
	read_startup_time();
	initGdt();
	PIC_remap(0x20, 0x28);
	initIdt();
	disable_translation();
	switch_scancode_set(2);
	display_time();
	terminal_writestring("PygmaliOS is up and running!\n");
	print_os_name();
	terminal_shell_set();
	//bad_time();
	while(1){
		screen_writer();
		msleep(10);
	}
}