#ifndef __SYSTEM_TREE_ROOT_H
#define __SYSTEM_TREE_ROOT_H

#include "system_tree/system_tree_branch.h"
#include "system_tree/system_tree_simple_branch.h"

/// @brief A simple System Tree Branch class for the root of the tree
///
/// The root of the tree contains all other elements of the tree.
class system_tree_root : public system_tree_simple_branch
{
public:
  /// @brief Standard constructor
  system_tree_root();

  /// @brief Standard destructor
  virtual ~system_tree_root();

private:
  /// Used to ensure that only one instance of the root of the tree exists at once.
  static uint32_t number_of_instances;
};

#endif
