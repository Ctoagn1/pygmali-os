#!/bin/bash

dd if=/dev/zero of=disk.img bs=1M count=64

parted disk.img --script mklabel gpt
parted disk.img --script mkpart PYGMALI_OS fat32 1MiB 100%
loopdevice=$(sudo losetup --find --show --partscan disk.img) 
partition=${loopdevice}p1
sudo mkfs.vfat -F 32 "$partition"
sudo mount "$partition" /mnt
sudo cp pygmali.ker /mnt
printf '\xB0\x07' > signature
sudo dd if=signature bs=1 seek=508 of="$partition" #put signature in first sector
rm signature
sudo dd if=build/firstsector.o of="$loopdevice" conv=notrunc #replaces mbr
sudo dd if=build/newboot.o of="$partition" seek=1 conv=notrunc
sudo umount /mnt
sudo losetup -d "$loopdevice"