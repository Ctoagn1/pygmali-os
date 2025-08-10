# pygmali-os
A mini-kernel I'm doing to try and learn more C and low-level development. 

## Features-
Uses ATA to support a FAT32 file system with a working directory and navigation- supports cd, ls, cat, pwd, touch, and mkdir. Make your own text files with the built-in text editor, galatea, by typing  `galatea <filename>`

![alt text](https://github.com/Ctoagn1/pygmali-os/blob/master/img/macbeth.png?raw=true)

It also uses the programmable interval timer to interface with the pc speaker- script simple monotonic melodies, and play them back with `orpheus <filename>`

![alt text](https://github.com/Ctoagn1/pygmali-os/blob/master/img/music.png?raw=true)

Reads CMOS for current time/date, and uses rtc to update it locally

**Full command list:** `echo <words>, help, play <note> <duratrion>, clear, time, pwd, ls <optional directory>, cd <directory>, rm <file> (with -r flag for dirs), cat <filename>, touch <filename>, mkdir <dirname>, galatea <filename, new or existing>, heapcheck, orpheus <filename>`

![alt text](https://github.com/Ctoagn1/pygmali-os/blob/master/img/homescreen.png?raw=true)

**TO DO:** I'm satisfied with the current state of the project for now, but for future additions:

-add paging

-add userspace (right now everything operates at ring 0)

-possibly create own bootloader rather than relying on grub

-add handling for specific exceptions, rather than register dump and kernel panic

-nic support? long ways away
