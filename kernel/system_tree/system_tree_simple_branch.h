/// @file
/// @brief Declare a simple system tree branch that tracks its children in a straightforward way.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "klib/klib.h"
#include "system_tree/system_tree_branch.h"

/// @brief A simple System Tree Branch class that can be used as a basis for others.
class system_tree_simple_branch : public ISystemTreeBranch
{
public:
  system_tree_simple_branch();
  virtual ~system_tree_simple_branch();

  virtual ERR_CODE get_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;
  virtual ERR_CODE add_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> child) override;
  virtual ERR_CODE create_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;
  virtual ERR_CODE rename_child(const std::string &old_name, const std::string &new_name) override;
  virtual ERR_CODE delete_child(const std::string &name) override;
  virtual std::pair<ERR_CODE, uint64_t> num_children() override;
  virtual std::pair<ERR_CODE, std::vector<std::string>>
    enum_children(std::string start_from, uint64_t max_count) override;

protected:
  /// Stores the child leaves of this branch.
  std::map<std::string, std::shared_ptr<ISystemTreeLeaf>> children;

  /// @brief Create a child on this branch.
  ///
  /// Called by the default system_tree_simple_branch::create_child() function to allow derivative classes to create a
  /// new child of this branch. This is a virtual function to allow derivative classes to create an appropriately typed
  /// child without having to replicate all the logic of create_child().
  ///
  /// Naming of the child is dealt with by the create_child() function.
  ///
  /// @param[out] child A pointer to the newly created child object.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE create_child_here(std::shared_ptr<ISystemTreeLeaf> &child);

  /// @brief Retrieve a child branch.
  ///
  /// If a child branch of the specified name exists, it is returned to the caller, otherwise a nullptr is returned.
  ///
  /// This is not a public function as most code outside system tree shouldn't be requesting only branches, but it is
  /// quite useful for system tree code to be able to get a branch object if one exists, rather than having to get the
  /// child and cast it to a branch type each time.
  ///
  /// This function only returns direct child branches, not grandchild (or deeper) branches.
  ///
  /// @param name The name of branch to get.
  ///
  /// @return The branch object, if name represents a branch. False otherwise.
  virtual std::shared_ptr<ISystemTreeBranch> get_child_branch(const std::string &name);

  /// @brief Lock protecting operations on this branch.
  kernel_spinlock child_tree_lock;
};
