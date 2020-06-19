kernel_compile_flags = [ # Flags for all C-like builds
  '-Wall',
  '-nostdinc',
  '-nostdlib',
  '-nodefaultlibs',
  '-mcmodel=large',
  '-ffreestanding',
  '-fno-exceptions',
  '-funwind-tables',
  '-U _LINUX',
  '-U _WIN32',
  '-U __linux__',
  '-D __AZALEA__',
  '-D __AZ_KERNEL__',
  '-D KL_TRACE_BY_SERIAL_PORT',
  '-Wno-unused-command-line-argument',
  '-Werror',

  # These flags are needed to use clang on Windows, but that doesn't work very well at the moment.
  #'--target=x86_64-elf',
  #'-D __ELF__',
  #'-D _GNU_SOURCE=1',

  # Uncomment this define to include a serial port based terminal as well as the normal VGA/keyboard one. This is
  # normally disabled as the locking in the kernel isn't great at the moment so it's a bit unstable.
  #'-D INC_SERIAL_TERM',
]

kernel_cxx_flags = [ # Flags specific to C++ builds
  '-nostdinc++',
  '-std=c++17',
  '-D _LIBCPP_NO_EXCEPTIONS',
]

user_compile_flags = [
  '-Wall',
  '-Werror',
  '-nostdinc',
  '-nostdlib',
  '-mcmodel=large',
  '-U _LINUX',
  '-U __linux__',
  '-D __AZALEA__',
]

user_cxx_flags = [
  '-std=c++17',
  '-nostdinc++',
]
