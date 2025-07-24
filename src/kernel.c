#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "tty.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboardhandler.h"



void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();
	initGdt();
	PIC_remap(0x20, 0x28);
	initIdt();
	disable_translation();
	switch_scancode_set(2);
	terminal_writestring("Pine is up and running!\n");
	terminal_shell_set();
	while(1){
		screen_writer();
	}
}