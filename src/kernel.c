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
#include "process.h"
#include "kmalloc.h"
#include "diskreader.h"
#include "writingmode.h"
#include "fatparser.h"
#include "paging.h"
void kernel_main()
{
	initGdt();
	PIC_remap(0x20, 0x28);
	scan_mbr();
	paging_setup();
	initIdt();
	read_boot_record();
	/* Initialize terminal interface */
	set_hertz(1000);
	initialize_scheduling();
	unmask_timer();
	terminal_initialize();
	psf_loader();
	read_startup_time();
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