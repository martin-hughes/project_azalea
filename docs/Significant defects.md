# Project Azalea significant defects

This document contains a non-exhaustive list of defects and deficiencies in the Azalea kernel. Simple bugs do not
belong in this list - use the Github issue tracker. Similarly, obviously missing or deliberately incomplete pieces of
code do not belong in the issue tracker - they belong here. Anything that hasn't been written yet (e.g. sound card
drivers) doesn't belong in either list.

The aim is to reduce this list over time. Please add any extras you find to it.

You will see 'known defects' list in several source files. They can be used for smaller issues limited to one or two
files.

## Synchronisation:

- Kernel mutexes and semaphores have no specific tests.
- Maximum waiting times have no tests.
- It's not clear what happens if a process is waiting for an object but then is killed before the wait is complete.

## Filesystems:

- A unified file system test suite is needed.
- FAT behaves very oddly with short names (they become capitalised upon created, which can confuse the kernel)