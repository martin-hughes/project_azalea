# Project Azalea global configuration.
#
# It is recommended that you do not edit these values in this file. Instead, set these values on the command line. They
# will then be remembered for future builds.

# Should the test code attempt to do memory leak detection? On Linux, this will use the Address Sanitizer that comes
# with Clang. On Windows, this will use the MSVC runtime memory leak detection methods. This helps detect leaks, but
# doesn't detect all of them. For example, with ASAN, items still referenced via global variables don't show up.
# Valgrind can be used to find this kind of leak, but this flag must be set to False because it conflicts with
# Valgrind.
test_attempt_mem_leak_check = False

# Folder that is the root of a filesystem for an Azalea system - imagine that if you were running Linux, it would be a
# folder you could chroot too. When built, the important system files end up here.
sys_image_root = "../azalea_sys_image"
