CCOM = i686-elf-gcc
ASM = nasm
GRUBMAKE = grub-mkrescue

SRCDIR := src
ASRCDIR := asmsrc
INCDIR := include
OBJDIR := build

C_SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(C_SRCS)) $(OBJDIR)/gdtfind.o $(OBJDIR)/isr_wrapper.o $(OBJDIR)/c_init.o $(OBJDIR)/header.o

KERNEL = pygmali.ker
ISO_DIR = isodir

CFLAGS = -std=gnu99 -ffreestanding -O0 -g -Wall -Wextra -I$(INCDIR)
LDFLAGS = -T linker.ld -ffreestanding -O0 -nostdlib -lgcc
AFLAGS = -f elf32

.PHONY: all clean iso run

all: $(KERNEL)
$(info OBJS = $(OBJS))
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	bear -- $(CCOM) $(CFLAGS) -c $< -o $@


$(OBJDIR)/gdtfind.o: $(ASRCDIR)/gdtfind.s | $(OBJDIR)
	$(ASM) $(AFLAGS) $< -o $@

$(OBJDIR)/c_init.o: $(ASRCDIR)/c_init.s | $(OBJDIR)
	$(ASM) $(AFLAGS) $< -o $@

$(OBJDIR)/header.o: $(ASRCDIR)/header.s | $(OBJDIR)
	$(ASM) $(AFLAGS) $< -o $@

$(OBJDIR)/isr_wrapper.o: $(ASRCDIR)/isr_wrapper.s | $(OBJDIR)
	$(ASM) $(AFLAGS) $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(CCOM) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

fs: $(KERNEL)
	./makeiso.sh

run : 
	qemu-system-i386 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -cdrom pygmali.iso -m 512M

clean:
	rm -rf $(OBJDIR) $(KERNEL) $(ISO_DIR) pygmalios.iso
