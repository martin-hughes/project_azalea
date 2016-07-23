qemu-system-x86_64 -hda kernel_disc_image_nonlive.vdi -D /tmp/qemu.log -no-reboot -d int,exec -debugcon stdio -smp cpus=2 -cpu Haswell,+x2apic
# Add -s -S to pause at start and enable a GDB server.

