#include "memory.h"
#include "klib/panic/panic.h"

// This file contains definitions for the operators new and delete. They refer to kmalloc / kfree directly.

// Ensure that these do not accidentally get included in the test scripts, as carnage occurs.
#ifndef AZALEA_TEST_CODE

void *operator new(unsigned long size)
{
  return kmalloc(size);
}

void *operator new[](unsigned long size)
{
  return kmalloc(size);
}

void operator delete(void *unlucky) noexcept
{
  kfree(unlucky);
}

void operator delete (void *unlucky, unsigned long)
{
  kfree(unlucky);
}

void operator delete[](void *unlucky) noexcept
{
  kfree(unlucky);
}

namespace std
{
void __throw_bad_alloc()
{
  panic("Bad allocation!");
}
}

#endif
