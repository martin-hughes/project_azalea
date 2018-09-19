#ifndef __SYSTEM_TREE_SIMPLE_BRANCH_H
#define __SYSTEM_TREE_SIMPLE_BRANCH_H

#include "klib/klib.h"
#include "system_tree/system_tree_branch.h"

#include <memory>

/// @brief A simple System Tree Branch class that can be used as a basis for others.
class system_tree_simple_branch : public ISystemTreeBranch
{
public:
  system_tree_simple_branch();
  virtual ~system_tree_simple_branch();

  ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type) override;
  ERR_CODE get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) override;
  ERR_CODE get_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;

  ERR_CODE add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch) override;
  ERR_CODE add_leaf (const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf) override;

  ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) override;
  ERR_CODE delete_child(const kl_string &name) override;

  virtual ERR_CODE create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) override;
  virtual ERR_CODE create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) override;

protected:
  /// Stores the child branches of this branch.
  kl_rb_tree<kl_string, std::shared_ptr<ISystemTreeBranch>> child_branches;

  /// Stores the child leaves of this branch.
  kl_rb_tree<kl_string, std::shared_ptr<ISystemTreeLeaf>> child_leaves;

  /// @brief Create a leaf on this branch.
  ///
  /// Called by the default system_tree_simple_branch::create_leaf() function to allow derivative classes to create a
  /// new leaf of this branch. This is a virtual function to allow derivative classes to create an appropriately typed
  /// leaf without having to replicate all the logic of create_leaf().
  ///
  /// This function will return a leaf with one reference acquired on it. The reference is owned by the calling
  /// function.
  ///
  /// Naming of the leaf is dealt with by the create_leaf() function.
  ///
  /// @param[out] leaf A pointer to the newly created leaf object.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE create_leaf_here(std::shared_ptr<ISystemTreeLeaf> &leaf);
};

#endif
