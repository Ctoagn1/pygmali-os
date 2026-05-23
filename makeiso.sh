mkdir -p isodir/boot/grub
cp pygmali.ker isodir/boot/pygmali.ker
cp grub.cfg isodir/boot/grub/grub.cfg
cd initrd
 find . | cpio -o -H newc > ../isodir/boot/initrd.cpio
 cd ..
grub-mkrescue -o pygmali.iso isodir