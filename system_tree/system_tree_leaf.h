#ifndef __SYSTEM_TREE_LEAF_INTERFACE_H
#define __SYSTEM_TREE_LEAF_INTERFACE_H

/// @brief The interface for leaves in System Tree
///
/// Very simple, the leaf must present an enclosed object when requested, via `get_leaf_object`. It is the user's
/// responsibility to determine the type of that object and to use it in the correct way.
class ISystemTreeLeaf
{
public:

  /// @brief Return the object underlying this leaf
  ///
  /// A leaf represents an arbitrary object stored in the System Tree. The user of that object accesses it via this
  /// function.
  /// 
  /// @return A pointer to the object. The caller should not `delete` the object.
  virtual void *get_leaf_object() = 0;
};

#endif
