# The Kernel's Basic Architecture

## Table of Contents

- [Overview](#overview)
- [Main components](#main-components)
- [Minor components](#minor-components)
- [External components](#external-components)

## Overview

The kernel of Project Azalea is a pure 64-bit system intended to run on modern Intel CPUs. So far as possible, it is
intended to uses new C++ features, where those do not introduce external dependencies. It has support for pre-emptive
SMP and fully separated memory spaces for each process.

There are several main components, and some more minor ones, all described below.

## Main components

### Memory manager (mem)

The memory manager is responsible for controlling both the physical and virtual memory available to the kernel, and
mapping between the two. It usefully provides large allocations of both, but for small or short-lived objects it is
better to use the `kmalloc` and `kfree` functions provided by the [Kernel Support Library](#kernel-support-library)  

### Processor controller (processor)

The Processor component provides control over the system's processors and directly related functions, such as:

- Stopping and starting processors
- Interrupt handling, including controlling the PICs
- High precision timing
- Inter-processor signalling

### Object Manager (object_mgr)

The Object Manager is responsible for correlating handles to objects. Users can then get a reference to the object by
requesting the Object Manager return it using the correct handle.

A subcomponent of the Object Manager is the Handle Manager, which keeps track of the allocation and deallocation of
handles, but doesn't do the actual correlation.

### Kernel Support Library (klib)

The Kernel Support Library (klib, for short) contains code that is universally useful throughout the kernel. For
example:

- Management of small memory allocations
- Basic data structures
- Synchronisation primatives
- Kernel debug tracing
- The all-important `panic()`

## Minor components

### ACPI interface (acpi)

This is a straightforward wrapper around the [ACPICA](#acpica) component. It also implements the OS Support Library,
which is the means by which ACPICA requests various actions of the host kernel.

### Kernel entry point (entry)

Does what it says on the tin. Controls the kernel's initial startup procedure, and once it's ready hands off to the
rest of the system.

### Security manager (security)

Doesn't do anything at the moment! The intention is for this component to provide all access control related functions.
However, at the moment there aren't any users, or any functions to control access to.

### System call interface (syscall)

Provides the system call interface. One day this will probably migrate to being a Major Component, but at the moment
there are so few system calls working that it's pointless. A list of system calls can be found in `syscall/syscall.h`.

There are two parts to the system call library - the kernel side and the user side. Both are contained in the `syscall`
directory, and which is which should be self-explanatory. Don't attempt to compile both into one project!

The system call binary interface similar to the Linux one - the call number is passed in RAX, then the arguments in
RDI, RSI, RDX, R10, R8, and R9, then the stack as needed. The return value is passed in RAX. Only RBP, RBX, and R12–R15
are preserved.

## External components

### ACPICA

ACPICA is a standard library for dealing with ACPI-compliant systems. It provides a large range of functionality from
managing the ACPI tables right through power management.

More information can be found on their [website](https://www.acpica.org/)

### vsnprintf

vsnprintf is an implementation of the `sprintf` family of functions. It is provided under a permissive licence, and
originates from ftp://ftp.jhweiss.de/pub/users/weiss/snprintf/snprintf.c, as of 2016-07-27