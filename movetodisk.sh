dd if=/dev/zero of=disk.img bs=1M count=64

parted disk.img --script mklabel msdos
parted disk.img --script mkpart primary fat32 1MiB 100%
sudo losetup -fP disk.img
loopdevice=$(sudo losetup --find --show --partscan disk.img) 
partition=${loopdevice}p1
sudo mkfs.vfat -F 32 "$partition"
sudo mount "$partition" /mnt
sudo mkdir -p /mnt/boot/grub
sudo cp pygmalios.kernel /mnt/boot
sudo cp grub.cfg /mnt/boot/grub
sudo grub-install --target=i386-pc --boot-directory=/mnt/boot "$loopdevice"
sync
sudo umount /mnt
sudo losetup -d "$loopdevice"