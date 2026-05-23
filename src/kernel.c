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
#include "vfs.h"
#include "ramfs.h"
#define KERNEL_OFFSET 0xC0000000
struct multiboot_tag_framebuffer fb;
struct multiboot_tag_mmap *mmap;
uint32_t initrd_start;
uint32_t initrd_end;
extern uintptr_t _stack_bottom;
extern uintptr_t _stack_top;
extern uintptr_t _heap_end;
extern uintptr_t _heap_start;


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
		if(tag->type==3){
			struct multiboot_tag_module* module = (struct multiboot_tag_module*)tag;
			initrd_end = module->mod_end + KERNEL_OFFSET;
			initrd_start = module->mod_start + KERNEL_OFFSET;
		}
		tag += (tag->size+7)/8;
	}
}
void kernel_main(uint32_t magic, struct multiboot_tag* addr)
{
	if(magic!=0x36d76289) return;
	multiboot_iterate(addr);
	initGdt();
	PIC_remap(0x20, 0x28);
	paging_setup(mmap);
	initIdt();
	terminal_initialize(&fb);
	printf("KERNEL STACK BASE = %x TOP = %x\n", &_stack_bottom, &_stack_top);
	printf("KERNEL HEAP TOP = %x END = %x\n", &_heap_start, &_heap_end);
	/* Initialize terminal interface */
	set_hertz(1000);
	terminal_writestring("PygmaliOS is up and running!\n");
	fs_instance_t* fs = vfs_mount("/", NULL, &ramfs_driver);
	if(load_cpio_into_ramfs((void*)initrd_start, fs, initrd_end-initrd_start)!=0){
		panic("FAILED TO LOAD INITRAMFS");
	}
	char* buf = kmalloc(100);
	vfs_read(resolve("/text.txt"), buf, 100, 0);
	terminal_writestring(buf);
	tty_init();
	print_os_name();
	initialize_scheduling();
	read_startup_time();
	disable_translation();
	switch_scancode_set(2);
	display_time();
	unmask_timer();
}

