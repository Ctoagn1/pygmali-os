mkdir -p isodir/boot/grub
cp pygmali.ker isodir/boot/pygmali.ker
cp grub.cfg isodir/boot/grub/grub.cfg
find initrd | cpio -o -H newc > isodir/boot/initrd.cpio
grub-mkrescue -o pygmali.iso isodir