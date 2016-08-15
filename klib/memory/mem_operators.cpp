#include "memory.h"

// This file contains definitions for the operators new and delete. They refer to kmalloc / kfree directly.

// TODO: Do we need to do something to call the constructor / destructor? (BUG)
void *operator new(unsigned long size)
{
  return kmalloc(size);
}

void *operator new[](unsigned long size)
{
  return kmalloc(size);
}

void operator delete (void *unlucky)
{
  kfree(unlucky);
}

// TODO: C++14 has a sized delete operator. At the moment, we don't keep track of allocation sizes, so there's no way
// to check that the delete size is correct. Maybe it should be added.
void operator delete (void *unlucky, unsigned long)
{
  kfree(unlucky);
}
