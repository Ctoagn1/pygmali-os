mkdir -p isodir/boot/grub
cp pygmali.ker isodir/boot/pygmali.ker
cp grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o pygmali.iso isodir