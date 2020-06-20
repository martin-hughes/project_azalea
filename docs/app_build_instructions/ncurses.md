# Building ncurses for Azalea

These instructions assume you're using a Linux machine, and have a copy of the ncurses codebase on it already. These
instructions were built using ncurses 6.1.

*Note: ncurses support in Azalea is not yet complete, you will probably not successfully compile any programs linking
to ncurses.*

Simply execute the following commands, after editing the paths to reflect your system. It is permissible to do an out-
of-tree build for ncurses.

```
export LDFLAGS='-L/xxx/azalea_sys_image/apps/developer/libc/lib -L/xxx/azalea_sys_image/apps/developer/kernel/lib -static'
export LIBS='-lazalea_linux_shim -lazalea_libc -lazalea'
export CFLAGS='-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include'
export CXXFLAGS='-Wall -mno-red-zone -nostdinc -nostdinc++ -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include -isystem /xxx/azalea_sys_image/apps/developer/libcxx-kernel/include/c++/v1 -static'
export CPPFLAGS='-Wall -mno-red-zone -nostdinc -nostdinc++ -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include -isystem /xxx/azalea_sys_image/apps/developer/libcxx-kernel/include/c++/v1 -static'
mkdir azalea_build
cd azalea_build
../configure --host=x86_64-elf --with-build-cc=clang --without-ada --disable-db-install --without-manpages --without-progs --without-tack --without-tests --with-build-cc=clang --without-cxx-binding --prefix=/xxx/azalea_sys_image/apps/developer/ncurses --includedir=/xxx/azalea_sys_image/apps/developer/ncurses/include --libdir=/xxx/azalea_sys_image/apps/developer/ncurses/lib cf_cv_working_poll='yes'
```

Manually edit `{build_dir}/include/ncurses_cfg.h` to include the following two lines (replacing the old definitions)

```
#define TERMINFO_DIRS "\\root\\apps\\developer\\ncurses\\terminfo"
#define TERMINFO "\\root\\apps\\developer\\ncurses\\terminfo"
```

Also unset HAVE_SIGACTION in the same file as follows:

```
#define HAVE_SIGACTION 0
```

In the function `make_dir_filename` (in read_entry.c), change the directory separator define from forward to backward
slashes.

Then execute

```
make
make install
```

Copy your system's terminfo database to `\root\apps\developer\ncurses` in the azalea system image tree. Also copy
`root\apps\developer\ncurses\include\ncurses\ncurses.h` up one level to the `include` directory.

## Notes:

1. When configuring we set `cf_cv_working_poll` because the ncurses configure script assumes path names that don't
   exist in Azalea and the test for poll() gives a false result.

2. You'll then need to copy the relevant output into a location to allow you to use it in while building Azalea programs.
