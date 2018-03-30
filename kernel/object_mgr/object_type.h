#ifndef __OBJECT_TYPE_H
#define __OBJECT_TYPE_H

#include "ref_counter.h"
#include "processor/processor.h"

// Stores and object and related data in the object manager.
struct object_data
{
  IRefCounted *object_ptr;
  task_thread *owner_thread;
  GEN_HANDLE handle;
};

#endif
