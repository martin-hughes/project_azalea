/// @file
/// @brief Declare system tree leaves

#pragma once

#include <stdint.h>
#include "object_mgr/handled_obj.h"

/// @brief The interface for leaves in System Tree
///
/// At present, there is no interface that is common to all Leaf objects. The only requirement to be a System Tree Leaf
/// is to inherit from this class.
class ISystemTreeLeaf : public IHandledObject
{
public:
  virtual ~ISystemTreeLeaf() {};

};
