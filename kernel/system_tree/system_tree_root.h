/// @file
/// @brief Declare the root of the system tree.

#pragma once

#include "system_tree/system_tree_branch.h"
#include "system_tree/system_tree_simple_branch.h"

/// @brief A simple System Tree Branch class for the root of the tree
///
/// The root of the tree contains all other elements of the tree. This object contains a single branch which itself
/// contains all the elements in the tree. This allows us to easily refer to the root of the tree with the pathname
/// '\\'
class system_tree_root : public ISystemTreeBranch
{
public:
  /// @brief Standard constructor
  system_tree_root();

  /// @brief Standard destructor
  virtual ~system_tree_root();

  // Overrides from ISystemTreeBranch
  virtual ERR_CODE get_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;
  virtual ERR_CODE add_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> child) override;
  virtual ERR_CODE create_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> &child) override;
  virtual ERR_CODE rename_child(const std::string &old_name, const std::string &new_name) override;
  virtual ERR_CODE delete_child(const std::string &name) override;
  virtual std::pair<ERR_CODE, uint64_t> num_children() override;
  virtual std::pair<ERR_CODE, std::vector<std::string>>
    enum_children(std::string start_from, uint64_t max_count) override;

private:
  /// Used to ensure that only one instance of the root of the tree exists at once.
  static uint32_t number_of_instances;

  /// The actual root of the system tree
  std::shared_ptr<system_tree_simple_branch> root;
};
