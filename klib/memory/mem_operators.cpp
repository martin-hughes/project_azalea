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
