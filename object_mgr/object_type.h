#ifndef __OBJECT_TYPE_H
#define __OBJECT_TYPE_H

// Stores and object and related data in the object manager.
struct object_data
{
  void *object_ptr;
  GEN_HANDLE handle;
  klib_list_item *owner_list_item;
};

#endif
