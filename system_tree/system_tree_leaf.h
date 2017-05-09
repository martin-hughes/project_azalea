#ifndef __SYSTEM_TREE_LEAF_INTERFACE_H
#define __SYSTEM_TREE_LEAF_INTERFACE_H

/// @brief The interface for leaves in System Tree
///
/// At present, there is no interface that is common to all Leaf objects. The only requirement to be a System Tree Leaf
/// is to inherit from this class.
class ISystemTreeLeaf
{
public:
  ~ISystemTreeLeaf() {};
};

#endif
