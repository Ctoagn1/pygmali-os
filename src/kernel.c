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
#include "writingmode.h"
#include "fatparser.h"
#include "multiboot.h"
#define HEAP_SIZE (32*1024*1024)

void kernel_main(multiboot_info_t* mbd, unsigned long magic)
{
	/*if(magic == MULTIBOOT_BOOTLOADER_MAGIC){ //checks if multiboot
		if(!((mbd->flags&0b01000000)==0b01000000)){ //is memory map valid?
			panic("MEMORY NOT FOUND");
		}
		for(int i=0; )
	}*/

	heap_start= (void*)ALIGN16((uint64_t) &_end);
	heap_end=heap_start+HEAP_SIZE;
	/* Initialize terminal interface */
	set_hertz(1000);
	terminal_initialize();
	scan_mbr();
	read_boot_record();
	read_startup_time();
	initGdt();
	PIC_remap(0x20, 0x28);
	initIdt();
	printf("HEAP BOUNDS: %p, %p\n", heap_start, heap_end);
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