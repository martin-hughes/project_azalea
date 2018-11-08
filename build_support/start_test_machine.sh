# There are two options for tracing. The first is via serial ports, the second is via the debug console.
# Which should be used depends on which one is compiled in via tracing.cpp.

qemu-system-x86_64 -drive file=fat:rw:fat-type=16:output/system_root,format=raw, -D /tmp/qemu.log -no-reboot -smp cpus=2 -cpu Haswell,+x2apic -serial stdio -device nec-usb-xhci -kernel output/system_root/kernel64.sys
#qemu-system-x86_64 -hda kernel_disc_image_nonlive.vdi -D /tmp/qemu.log -no-reboot -debugcon stdio -smp cpus=2 -cpu Haswell,+x2apic -device nec-usb-xhci
# Add -s -S to pause at start and enable a GDB server.

