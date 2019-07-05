#ifndef _KLIB_MEMORY_H
#define _KLIB_MEMORY_H

// If we're building the unit test program on Windows then add the Microsoft Debug Heap instrumentation to any source
// file using KLib. This won't instrument *everything* - but it's close enough.
#ifdef UT_MEM_LEAK_CHECK

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#define new DEBUG_NEW

#endif

#include <stdint.h>

#include "mem/mem.h"

void *kmalloc(uint64_t mem_size);
void kfree(void *mem_block);

uint64_t kl_mem_block_size(void *ptr);

// Only for use by test code. See the associated comment in memory.cpp for
// details.
#ifdef AZALEA_TEST_CODE
void test_only_reset_allocator();
#endif

// Useful memory-related helper functions.
void klib_mem_split_addr(uint64_t base_addr, uint64_t &page_addr, uint64_t &page_offset);

#endif
