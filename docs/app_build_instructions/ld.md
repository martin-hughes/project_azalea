# Building LD (the linker) for Azalea

These instructions assume you're using a Linux machine, and have a copy of the binutils codebase on it already. You
need the entire binutils source tree, sadly.

First execute the following commands, after editing the paths to reflect your system.

```
export LDFLAGS='-L/xxx/azalea_sys_image/apps/developer/libc/lib -L/xxx/azalea_sys_image/apps/developer/kernel/lib -L../zlib -static'
export LIBS='-lazalea_linux_shim -lazalea_libc -lazalea'
export CFLAGS='-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include'
export CXXFLAGS='-Wall -mno-red-zone -nostdinc -nostdinc++ -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include -isystem /xxx/azalea_sys_image/apps/developer/libcxx-kernel/include/c++/v1 -static'
./configure --host=x86_64-elf --disable-plugins --disable-shared --enable-targets=x86_64-elf --disable-gold --disable-gas --disable-nls --disable-sysinfo --disable-binutils --disable-bison --disable-flex --disable-gnat --disable-texinfo --disable-flex --disable-bison --disable-binutils --disable-gas --disable-fixincludes --disable-gcc --disable-cgen --disable-sid --disable-sim --disable-gdb --disable-gprof --disable-etc --disable-expect --disable-dejagnu --disable-m4 --disable-utils --disable-guile --disable-fastjar --disable-gnattools --disable-libcc1 --disable-gotools
make
```

The build will fail while building libiberty.

Edit `libiberty/config.h` to edit out the following lines:

```
#define HAVE_SYS_SYSCTL_H 1
#define HAVE_SYS_PRCTL_H 1
```

Add the following to the end of the same file:

```
#define DIR_SEPARATOR '\\'
```

Execute `make` again. This time it will fail while building bfd. Edit `bfd/config.h` to remove the line:

```
#define HAVE_FILENO 1
```

Execute `make` *again*. This time you should end up with the build succeeding. However, it has built a version of ld
that attempts to import dynamic libraries. The last command `make` executes should be a `libtool` command. Copy and
paste this command to your console, but immediately after `gcc` add the parameter `-all-static`. This should link
successfully, and now the file `ld/ld-new` is a version of ld compatible with Azalea.
