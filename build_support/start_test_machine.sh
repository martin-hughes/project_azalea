# There are two options for tracing. The first is via serial ports, the second is via the debug console.
# Which should be used depends on which one is compiled in via tracing.cpp.

qemu-system-x86_64 -hda kernel_disc_image_nonlive.vdi -D /tmp/qemu.log -no-reboot -d int,exec -smp cpus=2 -cpu Haswell,+x2apic -serial stdio
#qemu-system-x86_64 -hda kernel_disc_image_nonlive.vdi -D /tmp/qemu.log -no-reboot -d int,exec -debugcon stdio -smp cpus=2 -cpu Haswell,+x2apic
# Add -s -S to pause at start and enable a GDB server.

