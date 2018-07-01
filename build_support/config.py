# Project Azalea global configuration.
#
# Amend these to match your local configuration.

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