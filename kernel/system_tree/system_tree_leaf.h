#ifndef __SYSTEM_TREE_LEAF_INTERFACE_H
#define __SYSTEM_TREE_LEAF_INTERFACE_H

#include <stdint.h>
#include "object_mgr/ref_counter.h"

/// @brief The interface for leaves in System Tree
///
/// At present, there is no interface that is common to all Leaf objects. The only requirement to be a System Tree Leaf
/// is to inherit from this class.
class ISystemTreeLeaf : public IRefCounted
{
public:
  virtual ~ISystemTreeLeaf() {};

protected:
  // System tree leaves are, in general, managed and accessed through OM. So it makes sense to simply delete them when
  // we're finished with them.
  virtual void ref_counter_zero() { delete this; } ;
};

#endif
