#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tty.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboardhandler.h"
#include "rtc.h"



void kernel_main(void) 
{
	/* Initialize terminal interface */
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
	while(1){
		screen_writer();
	}
}