# Project Azalea - Readme

## Table of Contents

- [License](#License)
- [Background](#Background)
- [Choosing a Name](#Choosing a Name)
- [Requirements](#Requirements)
- [Getting Started](#Getting Started)
- [Contributing](#Contributing)
- [Find out more](#Find out more)

## Licence

This project is licensed under the MIT license, contained in the file named LICENSE. The license applies directly to
all parts of the software except that contained within the directory named external, to which individual and compatible
licences apply.

## Background

This project contains a very simple kernel written for a 64-bit Intel processor. At the moment, it is capable of
running fully independent, pre-emptively scheduled tasks on multiple processors. However, while it has a system call
interface, there are no system calls within it, so the system doesn't do anything interesting at the moment.

The project is a product of the author's curiousity, and is only really meant as an experimental system. You might find
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

Being a hobby project, there are some fairly specific requirements for this project to work:

- An x64 compatible processor. A recent Intel one should work.
- **Maximum** 1GB of RAM. This is a limitation of the memory manager.
- For compilation, A Linux machine running python 2.6 or better, GCC 5.4 or better, Scons, and NASM (Note: It may be
possible to do this on the new Windows Subsystem for Linux, this is untested)
- Optionally, doxygen for generating detailed code documentation. 

The project is routinely tested in qemu-system-x86_64 and Virtual Box, and very rarely on real hardware. Real hardware
bugs would be interesting to hear about!

## Getting Started

To get the system running from its current state, do the following:

- Get a copy of the code on your system, using your favourite method
- Follow the instructions in `extras/configure_disk_images.txt`
- From the root directory, execute `python build_support/build_script.py`
- Execute `build_support/start_test_machine.sh`

The system should now start! If you get a blue screen, that's a bug. Don't be surprised if not much actually happens.

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
2. The Markdown documentation in [System Architecture.md](./System Architecture.md) and `docs/components/`