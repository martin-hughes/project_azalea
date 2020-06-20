#!/bin/bash
# Create a disk image that can be used to boot Azalea.
#
# Requires the following packages - on Ubuntu at least. Your distro might vary.
# qemu (for qemu-nbd)
# VirtualBox (for vboxmanage)
#
# Also needs to be run as root - basically all these commands require privileged access. This script deliberately
# doesn't attempt to raise its own privileges so as to avoid any accidental mistakes on your machine. Exercise caution!

#######################################################################################################################
# NOTE: This script is not maintained. It will need editing for your system before it works correctly.                #
#######################################################################################################################

OUTPUT_FOLDER=output/
DISK_IMAGE_NAME=output/azalea_disk_image.vdi
NBD_DEVICE=/dev/nbd0
NBD_PARTITION=/dev/nbd0p1
TEMP_MOUNT_POINT=output/disk_mount
TEMP_MAP_FILE=output/grubmap
SYSTEM_ROOT=output/system_root

# Create the output folder, if it doesn't already exist.
if [ ! -d "$OUTPUT_FOLDER" ]; then
  mkdir "$OUTPUT_FOLDER"
fi

# Start by creating a blank, 32MB VDI file.
if [ ! -e "$DISK_IMAGE_NAME" ]; then
  vboxmanage createvdi --filename $DISK_IMAGE_NAME --size 32
else
  echo "Skipping disk image creation step - overwriting previous."
fi

# Mount this on the NBD system. Install the NBD module - it doesn't matter if this is done repeatedly.
modprobe nbd
qemu-nbd -c "$NBD_DEVICE" "$DISK_IMAGE_NAME"

# Create a new standard style partition table there, with a 31MB partition
parted --script "$NBD_DEVICE" mklabel msdos mkpart p fat16 1 31 set 1 boot on

# Format the partition in FAT format.
mkfs.fat "$NBD_PARTITION"

# Mount the newly formatted partition somewhere
if [ ! -d "$TEMP_MOUNT_POINT" ]; then
  mkdir "$TEMP_MOUNT_POINT"
fi
mount "$NBD_PARTITION" "$TEMP_MOUNT_POINT"

# Create the GRUB menu
mkdir -p ${TEMP_MOUNT_POINT}/boot/grub
cp build_support/grub_menu.cfg ${TEMP_MOUNT_POINT}/boot/grub/grub.cfg

# Tell GRUB what device we mean.
echo "(hd0) /dev/nbd0" > "$TEMP_MAP_FILE"

# Install GRUB on to the disk
grub-install --no-floppy \
             --modules="biosdisk part_msdos fat configfile normal multiboot" \
             --boot-directory="${TEMP_MOUNT_POINT}/boot" \
             --grub-mkdevicemap="$TEMP_MAP_FILE" \
              "$NBD_DEVICE"

# Copy all files into the disk
cp -r "$SYSTEM_ROOT"/*  "$TEMP_MOUNT_POINT"

# Tidy up.
rm "$TEMP_MAP_FILE"
umount "$TEMP_MOUNT_POINT"
qemu-nbd -d $NBD_DEVICE