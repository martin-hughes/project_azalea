#include "memory.h"

// This file contains definitions for the operators new and delete. They refer to kmalloc / kfree directly.

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

void operator delete (void *unlucky, unsigned long)
{
  kfree(unlucky);
}
