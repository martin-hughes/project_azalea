#ifndef __SYSTEM_TREE_BRANCH_INTERFACE_H
#define __SYSTEM_TREE_BRANCH_INTERFACE_H

#include <stdint.h>
#include <memory>

#include "object_mgr/handled_obj.h"
#include "user_interfaces/error_codes.h"
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
/// Some functionality that would be useful for all System Tree branches is included here.
///
/// It is not necessary for deriving classes to re-document the members of this interface unless there is anything
/// interesting to say.
class ISystemTreeBranch : public IHandledObject
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
  virtual ERR_CODE get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) = 0;

  /// @brief Get a pointer to the named leaf
  ///
  /// @param[in] name The name of the child leaf to return
  ///
  /// @param[out] leaf If the named leaf can be found, a pointer to it is stored in *leaf.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE get_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) = 0;

  /// @brief Add a branch to System Tree.
  ///
  /// @param name The name of the branch to add. The name must not conflict with any other branch or leaf that is a
  ///             child of this branch.
  ///
  /// @param branch The branch to add. The caller is responsible for not destroying the branch before removing it as a
  ///               child of this one.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch) = 0;

  /// @brief Add a leaf to System Tree.
  ///
  /// System Tree will acquire a reference to the leaf.
  ///
  /// @param name The name of the leaf to add. The name must not conflict with any other branch or leaf that is a
  ///             child of this branch.
  ///
  /// @param leaf The leaf to add. The caller is responsible for not destroying the leaf before removing it as a child
  ///             of this one.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE add_leaf (const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf) = 0;

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
  /// System Tree will release its reference to the child. The owner can then destroy it, provided that it is no longer
  /// needed.
  ///
  /// @param name The name of the child to remove.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE delete_child(const kl_string &name) = 0;

  /// @brief Splits a child's path name into the part referring to a child of this branch, and the remainder.
  ///
  /// Paths in System Tree are delimited by a \ character, so if `name_to_split` is of the form `[branch]\[rest]`, this
  /// function returns `[branch]` in `first_part` and `[rest]` in `second_part`.
  ///
  /// If there are no backslashes in `name_to_split` then `first_part` is set equal to `name_to_split` and
  /// `second_part` is set equal to "".
  ///
  /// @param[in] name_to_split The path to split as described above.
  ///
  /// @param[out] first_part The part of the path given that refers to a child branch of this one, split as described
  ///                        above.
  ///
  /// @param[out] second_part The remainder of the path, as described above.
  void split_name(const kl_string name_to_split, kl_string &first_part, kl_string &second_part) const
  {
    uint64_t split_pos = name_to_split.find("\\");

    if (split_pos == kl_string::npos)
    {
      first_part = name_to_split;
      second_part = "";
    }
    else
    {
      first_part = name_to_split.substr(0, split_pos);
      second_part = name_to_split.substr(split_pos + 1, kl_string::npos);
    }
  }

  /// @brief Create a new branch and add to System Tree.
  ///
  /// The branch that is created will be of a type determined by the implementer of this virtual function. The idea is
  /// that a filesystem will provide branches of the correct type for it.
  ///
  /// @param name The name of the branch to create. The name must not conflict with any other branch or leaf that is a
  ///             child of this branch.
  ///
  /// @param[out] branch A pointer to the newly created branch object.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch) = 0;

  /// @brief Create a new leaf and add to System Tree.
  ///
  /// The leaf that is created will be of a type determined by the implementer of this virtual function. The idea is
  /// that a filesystem will provide leaves of the correct type for it.
  ///
  /// The new leaf will have two references acquired on it. One is owned by System Tree, the other by the caller of
  /// this function.
  ///
  /// @param name The name of the leaf to create. The name must not conflict with any other branch or leaf that is a
  ///             child of this branch.
  ///
  /// @param[out] leaf A pointer to the newly created leaf object.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf) = 0;
};

#endif
