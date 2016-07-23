#ifndef _KLIB_MEMORY_H
#define _KLIB_MEMORY_H

#include "mem/mem.h"

void *kmalloc(unsigned int mem_size);
void kfree(void *mem_block);

// Only for use by test code. See the associated comment in memory.cpp for
// details.
void test_only_reset_allocator();

#endif
