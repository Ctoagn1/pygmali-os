CCOM = i686-elf-gcc
ASM = i686-elf-as
GRUBMAKE = grub-mkrescue

SRCDIR := src
INCDIR := include
OBJDIR := build

C_SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(C_SRCS)) $(OBJDIR)/boot.o $(OBJDIR)/gdtfind.o $(OBJDIR)/isr_wrapper.o

KERNEL = pygmalios.kernel
ISO_DIR = isodir

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(INCDIR)
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

.PHONY: all clean iso run

all: $(KERNEL)
$(info OBJS = $(OBJS))
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CCOM) $(CFLAGS) -c $< -o $@

$(OBJDIR)/boot.o: boot.s | $(OBJDIR)
	$(ASM) $< -o $@

$(OBJDIR)/gdtfind.o: gdtfind.s | $(OBJDIR)
	$(ASM) $< -o $@

$(OBJDIR)/isr_wrapper.o: isr_wrapper.s | $(OBJDIR)
	$(ASM) $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(CCOM) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

fs: $(KERNEL)
	./movetodisk.sh
iso: $(KERNEL)
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/
	cp grub.cfg $(ISO_DIR)/boot/grub/
	$(GRUBMAKE) -o pygmalios.iso $(ISO_DIR)

run : 
	qemu-system-i386 -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -hda disk.img

clean:
	rm -rf $(OBJDIR) $(KERNEL) $(ISO_DIR) pygmalios.iso