#ifndef __SYSTEM_TREE_SIMPLE_BRANCH_H
#define __SYSTEM_TREE_SIMPLE_BRANCH_H

#include "klib/klib.h"
#include "system_tree/system_tree_branch.h"

/// @brief A simple System Tree Branch class that can be used as a basis for others.
class system_tree_simple_branch : public ISystemTreeBranch
{
public:
  system_tree_simple_branch();
  virtual ~system_tree_simple_branch();

  ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type);
  ERR_CODE get_branch(const kl_string &name, ISystemTreeBranch **branch);
  ERR_CODE get_leaf(const kl_string &name, ISystemTreeLeaf **leaf);

  ERR_CODE add_branch(const kl_string &name, ISystemTreeBranch *branch);
  ERR_CODE add_leaf (const kl_string &name, ISystemTreeLeaf *leaf);

  ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name);
  ERR_CODE delete_child(const kl_string &name);

protected:
  /// Stores the child branches of this branch.
  kl_rb_tree<kl_string, ISystemTreeBranch *> child_branches;

  /// Stores the child leaves of this branch.
  kl_rb_tree<kl_string, ISystemTreeLeaf *> child_leaves;
};

#endif
