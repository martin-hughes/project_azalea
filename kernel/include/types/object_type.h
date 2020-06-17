/// @file
/// @brief Declare types useful to the object manager.

#pragma once

#include "handled_obj.h"

#include <memory>

/// @brief Stores an object and related data in the object manager.
///
struct object_data
{
  std::shared_ptr<IHandledObject> object_ptr; ///< Pointer to the object being mapped to a handle

  /// @brief The value of the handle corresponding to this object.
  ///
  /// This value should not be manipulated except by the object_manager class.
  GEN_HANDLE handle{0};

  /// @brief Data fields relevant to a handled object
  ///
  /// These fields affect how an object is viewed through the system call interface, but are not appropriate for
  /// storing within an object in the system tree. For example, files are represented by a single object in System Tree
  /// but each handle for a file is associated with a different seek position.
  ///
  /// It's a bit messy that these fields are stored even alongside objects that they're not valid for - frankly, this
  /// is a quick hack pending a proper rethink of how handles and System Tree objects relate to each other.
  struct
  {
    uint64_t seek_position{0}; ///< The current seek position of a file-like object.
  } data;
};
