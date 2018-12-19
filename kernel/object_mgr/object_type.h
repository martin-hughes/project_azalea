#ifndef __OBJECT_TYPE_H
#define __OBJECT_TYPE_H

#include "handled_obj.h"

#include <memory>

/// @brief Stores an object and related data in the object manager.
///
struct object_data
{
  std::shared_ptr<IHandledObject> object_ptr;
  GEN_HANDLE handle;
};

#endif
