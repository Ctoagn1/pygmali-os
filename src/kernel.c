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
#include "paging.h"
extern _heap_start;
#define HEAP_SIZE 1048576 //1 mib
void kernel_main()
{
	heap_start= (void*)ALIGN16((uint64_t) &_heap_start);
	heap_end=heap_start+HEAP_SIZE;
	initGdt();
	initIdt();
	PIC_remap(0x20, 0x28);
	scan_mbr();
	//paging_setup();
	read_boot_record();
	/* Initialize terminal interface */
	set_hertz(1000);
	terminal_initialize();
	psf_loader();
	read_startup_time();
	printf("HEAP BOUNDS: %p, %p\n", heap_start, heap_end);
	disable_translation();
	switch_scancode_set(2);
	display_time();
	terminal_writestring("PygmaliOS is up and running!\n");
	print_os_name();
	terminal_shell_set();
	while(1){
		screen_writer();
		msleep(10);
	}
}