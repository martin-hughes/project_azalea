kernel = [
    # Core components
    '#kernel/SConscript-main',
    '#kernel/SConscript-untested',
    '#external/SConscript-libtmt',

    # Devices
    '#kernel/devices/SConscript',
    '#kernel/devices/block/ata/SConscript',
    '#kernel/devices/block/proxy/SConscript',
    '#kernel/devices/block/ramdisk/SConscript',
    '#kernel/devices/generic/SConscript',
    '#kernel/devices/legacy/ps2/SConscript',
    '#kernel/devices/legacy/rtc/SConscript',
    '#kernel/devices/legacy/serial/SConscript',
    '#kernel/devices/pci/SConscript',
    '#kernel/devices/terminals/SConscript',
    '#kernel/devices/usb/SConscript',
    "#kernel/devices/virtio/SConscript",
  ]

x64_part = [
    '#kernel/arch/x64/SConscript',
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

list_program = [
    '#user/list/SConscript',
  ]

ncurses_program = [
    '#user/ncurses_test/SConscript',
  ]

user_mode_api = [
    '#user/libs/libazalea/SConscript',
  ]

main_tests = [
    '#kernel/SConscript-main',
    '#test/unit/test_core/SConscript',
    '#test/unit/SConscript-main',

    '#test/unit/dummy_libs/core_mem/SConscript',
    '#test/unit/dummy_libs/panic/SConscript',
    '#test/unit/dummy_libs/synch/Sconscript',
    '#external/SConscript-GoogleTest',

    # Devices for testing
    '#kernel/devices/SConscript', # Device Monitor and Core device interfaces.
    '#kernel/devices/block/ramdisk/SConscript',
    '#kernel/devices/block/proxy/SConscript',
    '#kernel/devices/generic/SConscript',
  ]

online_tests = [
    '#test/online/SConscript',
    '#external/SConscript-GoogleTest',
]
