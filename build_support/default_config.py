# Project Azalea global configuration.
#
# It is recommended that you do not edit these values in this file. Instead, create a file called "local_config.py" in
# the project root directory, and add your amended values there. This will help prevent the default config being
# changed in the git repository.

# As described in docs/Introduction.md, it is necessary to mount a disk image to copy the kernel file in to so that
# image can then be given to an virtual machine for testing. This is that location.
disk_image_folder = '/home/martin/disk_mount'

# The location of a C Library suitable for building Azalea programs. Azalea libc is recommended. If Azalea libc is
# being used, this should be the same as the value install_folder in the libc SConstruct file.
libc_location = '/home/martin/libc_build/'

# The location of header files corresponding to the C library being used. If Azalea libc is being used, this should be
# equal to libc_location + 'include'
libc_include = '/home/martin/libc_build/include'

# The location to install developer libraries and headers to. This is then used by the Azalea libc build script, where
# it is referenced in the similarly named variable azalea_header_folder
azalea_dev_folder = '/home/martin/azalea_dev'

# Should the test code attempt to do memory leak detection? On Linux, this will use the Address Sanitizer that comes
# with Clang. On Windows, this will use the MSVC runtime memory leak detection methods. This helps detect leaks, but
# doesn't detect all of them. For example, with ASAN, items still referenced via global variables don't show up.
# Valgrind can be used to find this kind of leak, but this flag must be set to False because it conflicts with
# Valgrind.
test_attempt_mem_leak_check = False

# Should we create temporary copies of memory-mapped files? When doing some filesystem tests, a disk image is mapped
# into RAM so that it can look a little bit like a live disk. However, not all host filesystems support memory mapping
# - for example, part of my Linux environment - so it is necessary to copy the image elsewhere. When set to True, the
# test code creates (and deletes) temporary copies of the virtual image in a location specified in the test code.
test_copy_mem_map_files = False
