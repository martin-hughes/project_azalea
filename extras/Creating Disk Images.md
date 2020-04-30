# How to create disk images to run Azalea in a VM

First, if you're using qemu, consider not creating disk images at all. See the start_demo.py script in the Azalea
Builder Project.

However, if you want to create disk images, here is my suggested procedure.

The main difficulty I had was installing Grub onto a disk image without mounting it using Linux. This presents some
problems on WSL, which lacks some of the necessary support. Instead, I created a bootable ISO with Grub on it and use
that to boot a 'plain' hard disk image.

## Creating the ISO.

- Create the following tree of directories in a convenient location: ./iso/boot/grub.
- Copy grub-example.cfg to ./iso/boot/grub/grub.cfg
- Execute grub-mkrescue -o grub.iso iso

## Creating a hard disk image.

There are two options. In both cases I've found the VHD (also known as vpc format to QEMU) to be easiest to use, as it
seems to be supported by the broadest cross section of tools and emulators.

- Use qemu-img. You'll then need to use something like qemu-nbd to create and mount the partitions.
- Use the Windows Disk Management tool. If you're using Windows you can then create and mount the partitions from this
  tool.

Then copy the entire contents of the Azalea system image created by the build tools to this partition.
