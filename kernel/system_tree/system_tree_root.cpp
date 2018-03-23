/// @file
/// @brief Implement `system_tree_root`, which handles the very root of the System Tree.

#include "klib/klib.h"
#include "system_tree/system_tree_root.h"

unsigned int system_tree_root::number_of_instances = 0;

system_tree_root::system_tree_root()
{
  KL_TRC_ENTRY;

  ASSERT(system_tree_root::number_of_instances == 0);
  system_tree_root::number_of_instances++;

  KL_TRC_EXIT;
}

system_tree_root::~system_tree_root()
{
  KL_TRC_ENTRY;

  // In some ways it'd be better if this destructor simply panic'd - but that'd confuse the test scripts. We need to be
  // able to destroy the system tree in order to demonstrate that no memory is leaked.
  KL_TRC_TRACE(TRC_LVL::ERROR, "System tree is being destroyed! Shouldn't occur\n");
  system_tree_root::number_of_instances--;

  KL_TRC_EXIT;
}
