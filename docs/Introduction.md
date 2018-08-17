# Project Azalea - Introduction

## Table of Contents

- [License](#license)
- [Background](#background)
- [Choosing a Name](#choosing-a-name)
- [Requirements](#requirements)
- [Getting Started - Full build](#getting-started---full-build)
- [Getting Started - Windows build](#getting-started---windows-builds)
- [Contributing](#contributing)
- [Find out more](#find-out-more)

## Licence

This project is licensed under the MIT license, contained in the file named LICENSE. The license applies directly to
all parts of the software except that contained within the directory named external, to which individual and compatible
licences apply.

## Background

This project contains a very simple kernel written for a 64-bit Intel processor. At the moment, it is capable of
running fully independent, pre-emptively scheduled tasks on multiple processors. However, while it has a system call
interface, there are no system calls within it, so the system doesn't do anything interesting at the moment.

The project is a product of the author's curiosity, and is only really meant as an experimental system. You might find
it interesting to see how the author resolved a problem that you had, or you might be interested in writing a specific
component of kernel code without having to start from scratch. You might not.

To quite Linus Torvalds, this is "just a hobby, won't be big and professional"

The project has its roots over a decade ago when the author got his first taste of programming. He was quickly keen to
find out if he could program the hardware directly, and with the help of [OSDev](http://wiki.osdev.org/) (Now a wiki)
and [Bona Fide OS Development](http://www.osdever.net/tutorials/) he was able to get simple code running in Bochs. At
the time though, he was inexperienced and lacking in debugging skills, so most of these projects didn't get much
further than doing some basic memory mapping and writing on the screen.

Nowadays, the author is all grown up and has several years professional programming experience under his belt (NOTE:
may not be apparent from the source code!) so when he was bored relatively recently he decided to try again, but
targeting a modern platform. The result is this project.

## Choosing a Name

Why "Project Azalea". Good question. For ages, the author was trying to think of a better name than "the kernel" or
"that damned thing". He felt that his project needed a name, but couldn't think of a decent name for the end product -
whatever that might end up being. Instead he decided to pick a codename, which doesn't need to be so representative of
what's being made. He quite likes the look of azalea plants, and they start with the letter "A", so it seemed like a
reasonable codename.

Sorry if you hate it.

## Requirements

Being a hobby project, there are some fairly specific requirements for this project to run:

- An x64 compatible processor. A recent Intel one should work.
- **Maximum** 1GB of RAM. This is a limitation of the memory manager.

To compile the full system, you will need the following tools and libraries installed:
- Azalea-libc - this can be built as part of compiling the kernel, as described below.
- Python 2.6 or later (*)
- GCC 5.4 or later
- Clang 3.8.0 or later
- SCons (*)
- NASM (*)
- Boost iostreams (*) - only used in the test scripts.
- Qemu - the default test script runs on qemu, and qemu-nbd is required to create disk images from scratch (which is
  optional). Depending on how you choose to mount this disk images you might need qemu-nbd.
- Virtualbox - Required to generate disk images from scratch (optional) and can be used as a test system.
- GRUB2 2.02 beta 2 or later - Required to generate disk images from scratch. (optional)
- Doxygen - Only needed to generate documentation. (optional)
- Visual Studio - only needed if doing a Windows build (optional, see below)

Items marked with a (*) are the only ones required if doing a unit test program build on Windows. 

Full compilation has been tested using Ubuntu 16.04. It may work on the Windows Subsystem for Linux, but is untested.

The project is routinely tested in qemu-system-x86_64 and Virtual Box, and very rarely on real hardware. Real hardware
bugs would be interesting to hear about!

## Getting Started - Full build

A full build requires a Linux toolchain, as described in [Requirements](#requirements) above. To get the system running
from scratch, do the following:

1. Get a copy of the code on your system, using your favourite method.
2. Also, get a copy of the Azalea-libc code.
3. Follow the instructions in `extras/configure_disk_images.txt`. Remember to mount the disk image whenever necessary
(e.g. after a reboot)
4. Configure the values in build_support/config.py and the Azalea-libc SConstruct file as appropriate.
5. From the root directory of the kernel, execute `scons install-headers` and check there are no errors.
6. In the Azalea-libc root directory, execute `scons` followed by `scons install` - again, check for errors.
7. Back in the kernel code tree, execute `scons` and check there are no errors.
8. Finally, execute `build_support/start_test_machine.sh`.

If you have already executed these steps, you will probably only need to execute steps 6-8 again if you make changes.
Execute step 5 again if you make changes to the user interface headers.

The system should now start! Assuming that everything went well, you will see two things:

- Various messages printed to the host machine's stdout. You can enable or disable these by defining (or not)
  ENABLE_TRACING in a file.
- A message printed to the display of the emulated machine.

If something goes wrong, you will get a blue screen of death - which is a bug so please let me know!

The system will start a program called 'initprog' from the root of its disk image and run it in user mode. At the
moment, 'initprog' is compiled from the source in `extras/demo_program` as part of the main build script.

## Getting Started - Windows Builds

At present, there is no support for building the live kernel, live test programs or C Library using Windows. It is
possible to compile the unit test program to run on Windows. If possible, further Windows build support will be added
in future, but there are a few obstacles to this, described below.

In order to get the unit test program running on Windows:

1. Get a copy of the code on your system, using your favourite method.
2. Configure the values in build_support/config.py as appropriate.
3. Ensure the INCLUDE and LIB environment variables are configured to include location of the Boost iostreams library.
4. Ensure the other environment variables are correctly configured - the easiest way is to start a Developer Command
   Prompt - this is one of the links in the Visual Studio start menu folder.
5. In the Azalea-libc root directory, execute `scons` and check there are no errors.
6. Execute output\main-tests.

This allows at least some development work to be done using a Windows machine. As mentioned, there are some obstacles
to a full Windows build - for example:

- The native Visual Studio tools don't seem to be able to produce ELF outputs (an issue for initprog)
- I'm not sure if there is a way to use custom linker scripts or not (to produce the kernel binary), but it'll
  definitely be a different format to the LD version.
- The compiler intrinsics are differently named.
- Windows uses a different calling convention, which breaks a lot of the assembly language files in the kernel code.

These can probably be overcome by using the Linux tools on Windows, but a large benefit of using the Visual Studio
toolchain is being able to easily do debugging within Visual Studio!

## Contributing

The author welcomes any thoughts, fixes or complaints via Github, as well as general conversation. He also welcomes
additions to functionality, although if it is for an area he was planning to work on anyway it might be respectfully
declined so as to preserve the challenge for himself. Note that you must be prepared for contributions to be licensed
under the MIT license - you will, of course, be credited.

You are more than welcome to clone this work for your own purposes - and if you make good progress please do get in
touch!

## Find out more

There are two other sources of documentation, although both are very much in development.

1. Code comments in the source files. You can gather these, and function call traces by running doxygen against
`extras/Docs-Doxyfile`. The output appears in `docs/html/`
2. The Markdown documentation in [System Architecture.md](System%20Architecture.md) and `docs/components/`