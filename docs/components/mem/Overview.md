# Memory Manager - Overview

## Contents

- [Introduction](#Introduction)
- [Physical Memory](#Physical Memory)
- [Virtual Memory](#Virtual Memory)
- [KLib Interface](#KLib Interface)
- [Minor Functions](#Minor Functions)

## Introduction

Low-level memory management is provided to the kernel by the 'mem' component - the Memory Manager. It is split in to
two main function groups, those tracking the allocation of physical memory, and those tracking allocation of virtual
memory. A small part of the code permits mapping of virtual memory addresses to physical addresses, but the user can
manage allocations of both independently if needed.

Virtual memory allocation is done on a per-process basis, since each process in the system uses a different set of page
tables. Physical memory allocation is system-wide, with no support for NUMA at present, since it is not possible for
two processes to store data at the same place in RAM without overwriting each other.

## Physical Memory

The physical memory manager is by far the simpler of the two parts of 'mem'. It maintains a bitmap with one bit for
each page of memory in the system, where the size of a page is system-dependent - but ultimately configured in the code
by constants at the top of `physical.cpp`.

When the system starts, each available page is represented in the bitmap by a bit set to one. Allocated pages have
their bit set to zero. At present, the physical memory manager does not attempt to determine whether a page being freed
is one that is genuinely being freed, or a physical page that was not found to exist at boot time and is now being
spuriously freed. It is up to the user to behave themselves!

## Virtual Memory

The Virtual Memory Manager's primary responsibility is tracking which parts of the virtual memory space are in use. It
does this using a Buddy Allocation System, the priciple of which is well described on 
[Wikipedia](https://en.wikipedia.org/wiki/Buddy_memory_allocation).

Additionally, it is responsible for managing the mappings between physical and virtual memory, the mechanism for which
is processor-specific.

## KLib Interface

The Kernel Support Library provides heap functions to the rest of the kernel. Unless the kernel needs to do specific
memory control operations, it should generally use the `new` and `delete` operators provided by KLib.

A description of how KLib operates the heap is provided in the KLib documentation. 

## Minor Functions

The Memory Manager provides a few other smaller functions:

- On x64, there is the concept of the Page Attribute Table (PAT) which specifies how operations that access specific
  memory mappings should be cached. The Memory Manager simply sets a fixed PAT and uses it throughout the x64 code.

- Also on x64, the Memory Manager ensures that all processes expose the same set of address mappings for the kernel.
  Instead of the kernel switching to its own private page table upon entering Ring 0 code, it continues to execute
  using the page table of the calling process. The means it must ensure that each process has its page tables updated
  as needed when the kernel mappings change. This code is handled in the file `mem_pml4-x64.cpp`, so named because to
  achieve this it is simply necessary to ensure that the upper half of each process's PML4 table (see the Intel docs
  for more information about the page table names) is synchronised.