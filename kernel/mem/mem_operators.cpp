/// @file
/// @brief Memory helper functions.
///
/// These are part of the normal specs, so are not documented further.

/// @cond

#include "mem.h"
#include "panic.h"

#include <stdlib.h>

// This file contains definitions for the operators new and delete. They refer to kmalloc / kfree directly.

// Ensure that these do not accidentally get included in the test scripts, as carnage occurs.
#ifndef AZALEA_TEST_CODE

extern "C" void *__memalign(size_t align, size_t len);

void *operator new(uint64_t size)
{
  return kmalloc(size);
}

void *operator new[](uint64_t size)
{
  return kmalloc(size);
}

void operator delete(void *unlucky) noexcept
{
  kfree(unlucky);
}

void operator delete (void *unlucky, uint64_t)
{
  kfree(unlucky);
}

void operator delete[](void *unlucky) noexcept
{
  kfree(unlucky);
}

/*
namespace std
{
void __throw_bad_alloc()
{
  panic("Bad allocation!");
}
}*/

void *malloc(size_t size)
{
  return kmalloc(size);
}

void *calloc(size_t num, size_t size)
{
  void *r = kmalloc(num * size);
  memset(r, 0, num * size);
  return r;
}

void free(void *ptr)
{
  kfree(ptr);
}

void* realloc (void* ptr, size_t size)
{
  void *newptr;
  size_t min_size = size;
  uint64_t old_size;

  if (ptr != nullptr)
  {
    old_size = kl_mem_block_size(ptr);
    if (min_size > old_size)
    {
      min_size = old_size;
    }
  }
  else
  {
    min_size = 0;
  }

  if (size != 0)
  {
    newptr = malloc(size);

    if (min_size != 0)
    {
      memcpy(newptr, ptr, min_size);
    }
  }
  else
  {
    newptr = nullptr;
  }

  if (ptr != nullptr)
  {
    free(ptr);
  }

  return newptr;
}

void *__memalign(size_t align, size_t len)
{
  INCOMPLETE_CODE("memalign");
}

#endif
/// @endcond
