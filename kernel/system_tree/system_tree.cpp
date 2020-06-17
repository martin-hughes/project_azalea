/// @file
/// @brief System Tree control functions
///
/// The System Tree is analogous to Linux's Virtual File System (VFS).

#include "system_tree.h"
#include "types/system_tree_root.h"
#include "panic.h"
#include "k_assert.h"

namespace
{
  system_tree_root *tree_root = nullptr;
}

/// @brief Initialise the System Tree.
///
/// After initialisation, the tree is completely empty.
void system_tree_init()
{
  ASSERT(tree_root == nullptr);
  tree_root = new system_tree_root();
}

/// @brief Return a pointer to the system tree
///
/// @return A pointer to the root of the system tree, if it exists. Otherwise fails.
// No tracing here, since this is a simple wrapper and will execute often.
system_tree_root *system_tree()
{
  ASSERT(tree_root != nullptr);
  return tree_root;
}

/// @brief Destroy the System Tree.
///
/// **This should never be called in production code!** It is for use within the test scripts only.
void test_only_reset_system_tree()
{
  delete tree_root;
  tree_root = nullptr;
}
