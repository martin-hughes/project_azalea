# Building nano for Azalea

These instructions assume that you have already successfully built the ncurses library for your Azalea system.

Then execute the following commands, after editing the paths to reflect your system.

```
export LDFLAGS='-L/xxx/azalea_sys_image/apps/developer/libc/lib -L/xxx/azalea_sys_image/apps/developer/kernel/lib -L/xxx/azalea_sys_image/apps/developer/ncurses/lib -static'
export LIBS='-lncurses -lazalea_linux_shim -lazalea_libc -lazalea'
export CFLAGS='-Wall -mno-red-zone -nostdinc -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include -isystem /xxx/azalea_sys_image/apps/developer/ncurses/include'
export CXXFLAGS='-Wall -mno-red-zone -nostdinc -nostdinc++ -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include -isystem /xxx/azalea_sys_image/apps/developer/libcxx-kernel/include/c++/v1 -isystem /xxx/azalea_sys_image/apps/developer/ncurses/include -static'
export CPPFLAGS='-Wall -mno-red-zone -nostdinc -nostdinc++ -nostdlib -nodefaultlibs -mcmodel=large -ffreestanding -fno-exceptions -U _LINUX -U __linux__ -D __AZALEA__ -isystem /xxx/azalea_sys_image/apps/developer/libc/include -isystem /xxx/azalea_sys_image/apps/developer/kernel/include -isystem /xxx/azalea_sys_image/apps/developer/libcxx-kernel/include/c++/v1 -isystem /xxx/azalea_sys_image/apps/developer/ncurses/include -static'

../configure --host=x86_64-elf --enable-tiny --disable-threads --disable-browser --disable-operatingdir --disable-libmagic --disable-utf8 --disable-nls ac_cv_func_glob=yes
```

Then make the following small patches to the nano source:

- Comment out the entire function 'execute_command' in text.c
- Comment out the include_next sys/wait.h in sys/wait.h
- Replace the contents of get_full_path with: `return real_dir_from_tilde(origpath);`
- Comment out #define HAVE_PWD_H 1
- Comment out the block after 'Get the state of standard input and ensure it uses blocking mode.' in nano.c
- Comment out contents of reconnect_and_store_state in nano.c
- In do_suspend, comment out the kill() call

Execute `make`, then copy src/nano to your Azalea system image.
