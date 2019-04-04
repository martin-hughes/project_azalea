kernel = [
    # Core components
    '#kernel/entry/SConscript',
    '#kernel/processor/SConscript-generic',
    '#kernel/processor/SConscript-x64',
    '#kernel/klib/memory/SConscript',
    '#kernel/klib/panic/SConscript',
    '#kernel/mem/SConscript-main',
    '#kernel/mem/SConscript-testable',
    '#kernel/klib/c_helpers/SConscript',
    '#kernel/klib/misc/SConscript',
    '#kernel/klib/data_structures/SConscript',
    '#kernel/klib/tracing/SConscript',
    '#kernel/klib/synch/SConscript',
    '#kernel/klib/synch/SConscript-messages',
    '#kernel/klib/synch/SConscript-spinlocks',
    '#kernel/syscall/SConscript-kernel',
    '#kernel/syscall/SConscript-kernel-x64',
    '#kernel/acpi/SConscript',
    '#external/SConscript-libcxxrt',
    '#kernel/object_mgr/SConscript',
    '#kernel/system_tree/SConscript',
    '#kernel/system_tree/process/SConscript',

    # Devices
    '#kernel/devices/block/ata/SConscript',
    '#kernel/devices/block/proxy/SConscript',
    '#kernel/devices/block/ramdisk/SConscript',
    '#kernel/devices/generic/SConscript',
    '#kernel/devices/legacy/ps2/SConscript',
    '#kernel/devices/legacy/rtc/SConscript',
    '#kernel/devices/pci/SConscript',
    '#kernel/devices/usb/SConscript',

    # Filesystems
    '#kernel/system_tree/fs/fat/SConscript',
    '#kernel/system_tree/fs/pipe/SConscript',
    '#kernel/system_tree/fs/mem/SConscript',
    '#kernel/system_tree/fs/proc/SConscript',
    '#kernel/system_tree/fs/dev/SConscript',
  ]

init_program = [
    '#user/init_program/SConscript',
  ]

shell_program = [
    '#user/simple_shell/SConscript',
  ]

echo_program = [
    '#user/echo/SConscript',
  ]

user_mode_api = [
    '#user/libs/libazalea/SConscript',
    '#kernel/syscall/SConscript-user',
  ]

main_tests = [
    '#kernel/klib/c_helpers/SConscript',
    '#kernel/klib/data_structures/SConscript',
    '#kernel/klib/memory/SConscript',
    '#kernel/klib/misc/SConscript',
    '#kernel/klib/synch/SConscript-messages',
    '#kernel/klib/synch/SConscript-spinlocks',
    '#kernel/klib/tracing/SConscript',
    '#kernel/mem/SConscript-testable',
    '#kernel/processor/Sconscript-generic',
    '#kernel/object_mgr/SConscript',
    '#kernel/syscall/SConscript-kernel',
    '#kernel/system_tree/SConscript',
    '#test/SConscript-main',
    '#test/dummy_libs/core_mem/SConscript',
    '#test/dummy_libs/panic/SConscript',
    '#test/dummy_libs/synch/Sconscript',
    '#test/test_core/SConscript',
    '#external/SConscript-GoogleTest',

    # Devices for testing
    '#kernel/devices/block/ramdisk/SConscript',
    '#kernel/devices/block/proxy/SConscript',
    '#kernel/system_tree/fs/pipe/SConscript',
    '#kernel/system_tree/fs/fat/SConscript',
    '#kernel/system_tree/fs/proc/SConscript',
    '#kernel/system_tree/fs/mem/SConscript',
  ]