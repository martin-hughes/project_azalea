#ifndef __SYSTEM_TREE_BRANCH_INTERFACE_H
#define __SYSTEM_TREE_BRANCH_INTERFACE_H

#include "klib/misc/error_codes.h"
#include "klib/data_structures/string.h"
#include "system_tree/system_tree_leaf.h"

/// @brief The type of a child of a branch in System Tree.
///
/// In System Tree, branches can contain two types of children. Other branches and leaves. Leaves cannot themselves
/// contain other branches. This type allows the caller to determine whether a given name in System Tree refers to a
/// branch or a leaf.
enum class CHILD_TYPE
{
  BRANCH,    ///< Named child is a branch
  LEAF,      ///< Named child is a leaf
  NOT_FOUND, ///< The named child could not be found.
};

/// @brief The interface which all branch implementations must implement.
///
/// System Tree is capable of storing any object that implements this interface, calling it a "branch". It is up to the
/// derived class how to implement this in a way that suits it best - for example, the implementation of an on-disk
/// filesystem would not necessarily match that of a virtual "proc"-like tree.
///
/// It is not necessary for deriving classes to re-document the members of this interface unless there is anything
/// interesting to say.
class ISystemTreeBranch
{
public:
  /// @brief Standard virtual destructor
  virtual ~ISystemTreeBranch() { };

  /// @brief Return the type of the named child
  ///
  /// @param[in] name The name of a child to look for in System Tree
  ///
  /// @param[out] type The type of the child. If the child does not exist, `type` is set to `NOT_FOUND`
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE get_child_type(const kl_string &name, CHILD_TYPE &type) = 0;

  /// @brief Get a pointer to the named branch
  ///
  /// @param[in] name The name of the child branch to return
  ///
  /// @param[out] branch If the named branch can be found, a pointer to it is stored in *branch.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE get_branch(const kl_string &name, ISystemTreeBranch **branch) = 0;

  /// @brief Get a pointer to the named leaf
  ///
  /// @param[in] name The name of the child leaf to return
  ///
  /// @param[out] leaf If the named leaf can be found, a pointer to it is stored in *leaf.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE get_leaf(const kl_string &name, ISystemTreeLeaf **leaf) = 0;

  /// @brief Add a branch to System Tree.
  ///
  /// @param name The name of the branch to add. The name must not conflict with any other branch or leaf that is a
  ///             child of this branch.
  ///
  /// @param branch The branch to add. The caller is responsible for not destroying the branch before removing it as a
  ///               child of this one.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE add_branch(const kl_string &name, ISystemTreeBranch *branch) = 0;

  /// @brief Add a leaf to System Tree.
  ///
  /// @param name The name of the leaf to add. The name must not conflict with any other branch or leaf that is a
  ///             child of this branch.
  ///
  /// @param leaf The leaf to add. The caller is responsible for not destroying the leaf before removing it as a child
  ///             of this one.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE add_leaf (const kl_string &name, ISystemTreeLeaf *leaf) = 0;

  /// @brief Rename a child of this branch.
  ///
  /// @param old_name The current name of the child, which will be changed to new_name.
  ///
  /// @param new_name The name to rename the child to. This name must not exist in the tree already.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE rename_child(const kl_string &old_name, const kl_string &new_name) = 0;

  /// @brief Remove the child from this branch.
  ///
  /// The owner can then destroy its contents, provided that it is no longer needed.
  ///
  /// @param name The name of the child to remove.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE delete_child(const kl_string &name) = 0;
};

#endif
