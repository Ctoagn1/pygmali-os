# pygmali-os
A mini-kernel written in C with FAT32 file system and hardware interaction.

## Features-
Uses ATA to support a FAT32 file system with a working directory and navigation- supports cd, ls, cat, pwd, touch, and mkdir. Make your own text files with the built-in text editor, galatea, by typing  `galatea <filename>`

![alt text](https://github.com/Ctoagn1/pygmali-os/blob/master/img/macbeth.png?raw=true)

It also uses the programmable interval timer to interface with the pc speaker- script simple monotonic melodies, and play them back with `orpheus <filename>`

![alt text](https://github.com/Ctoagn1/pygmali-os/blob/master/img/music.png?raw=true)

Reads CMOS for current time/date, and uses rtc to update it locally

**Full command list:** `echo <words>, help, play <note> <duratrion>, clear, time, pwd, ls <optional directory>, cd <directory>, rm <file> (with -r flag for dirs), cat <filename>, touch <filename>, mkdir <dirname>, galatea <filename, new or existing>, heapcheck, orpheus <filename>`

![alt text](https://github.com/Ctoagn1/pygmali-os/blob/master/img/homescreen.png?raw=true)

# Download
Dependencies- 
qemu-full, 
i386-elf-gcc (only if you want to rebuild it! disk image is already included)

First, clone the repo `https://github.com/Ctoagn1/pygmali-os` and then go into the project directory and type `make run`. Depending on your audio driver, the pa in `-audiodev pa,id=speaker` in the Makefile may need to be replaced with your existing driver, or `-audiodev pa,id=speaker -machine pcspk-audiodev=speaker` can be removed entirely if you don't care about audio.
For those on Linux who want to regenerate the disk image, `make fs` can be used. (Note that the script to regenerate the disk does require usage of mounting and the loop device, which requires sudo).

# TO DO: I'm satisfied with the current state of the project for now, but for future additions:

-set up 64 bit mode

-add userspace (right now everything operates at ring 0)

-external binary loading

-add handling for specific exceptions, rather than register dump and kernel panic

-nic support? long ways away

# Credits
A heavy thank you to the osdev wiki, `https://wiki.osdev.org`, without which most of this would have been next to impossible.
