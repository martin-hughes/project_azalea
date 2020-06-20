# Building NASM for Azalea

These instructions assume you're using a Linux machine, and have a copy of the NASM codebase on it already.

Simply execute the following commands, after editing the paths to reflect your system.

```
export LDFLAGS='-L/xxx/azalea_sys_image/apps/developer/libc/lib -L/xxx/azalea_sys_image/apps/developer/kernel/lib -static'
export LIBS='-lazalea_linux_shim -lazalea_libc -lazalea'
export CFLAGS='-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include'
export CXXFLAGS='-Wall -mno-red-zone -nostdinc -nostdinc++ -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include -isystem /xxx/azalea_sys_image/apps/developer/libcxx-kernel/include/c++/v1 -static'
./configure --host=azalea
make
```

You may see some errors at the end of the build, but will probably still leave a nasm executable in the root of the
source tree that you can copy to your Azalea system.
