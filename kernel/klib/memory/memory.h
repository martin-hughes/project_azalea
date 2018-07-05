#ifndef _KLIB_MEMORY_H
#define _KLIB_MEMORY_H

#include <stdint.h>

#include "mem/mem.h"

void *kmalloc(uint64_t mem_size);
void kfree(void *mem_block);

// Only for use by test code. See the associated comment in memory.cpp for
// details.
#ifdef AZALEA_TEST_CODE
void test_only_reset_allocator();
#endif

#endif
