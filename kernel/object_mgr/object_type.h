#ifndef __OBJECT_TYPE_H
#define __OBJECT_TYPE_H

// Stores and object and related data in the object manager.
struct object_data
{
  ISystemTreeLeaf *object_ptr;
  GEN_HANDLE handle;
};

#endif
