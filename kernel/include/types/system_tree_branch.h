/// @file
/// @brief Declare system tree branches

#pragma once

#include <string>
#include <stdint.h>
#include <memory>
#include <vector>

#include "types/handled_obj.h"
#include "azalea/error_codes.h"
#include "tracing.h"

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
class ISystemTreeBranch : public virtual IHandledObject
{
public:
  /// @brief Standard virtual destructor
  virtual ~ISystemTreeBranch() { };

  /// @brief Get a pointer to the named child
  ///
  /// @param[in] name The name of the child to return
  ///
  /// @param[out] child If the named child can be found, a pointer to it is stored in child.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE get_child(const std::string &name, std::shared_ptr<IHandledObject> &child) = 0;

  /// @brief Add a child to this branch of System Tree.
  ///
  /// @param name The name of the child to add. The name must not conflict with any other child of this branch.
  ///
  /// @param child The child to add.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE add_child(const std::string &name, std::shared_ptr<IHandledObject> child) = 0;

  /// @brief Create a new child and add to System Tree.
  ///
  /// The child that is created will be of a type determined by the implementer of this virtual function. The idea is
  /// that a filesystem will provide children of the correct type for it.
  ///
  /// @param name The name of the child to create. The name must not conflict with any other child of this branch.
  ///
  /// @param[out] child A pointer to the newly created child object.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE create_child(const std::string &name, std::shared_ptr<IHandledObject> &child) = 0;

  /// @brief Rename a child of this branch.
  ///
  /// @param old_name The current name of the child, which will be changed to new_name.
  ///
  /// @param new_name The name to rename the child to. This name must not exist in the tree already.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE rename_child(const std::string &old_name, const std::string &new_name) = 0;

  /// @brief Remove the child from this branch.
  ///
  /// System Tree will release its reference to the child. The owner can then destroy it, provided that it is no longer
  /// needed.
  ///
  /// @param name The name of the child to remove.
  ///
  /// @return An appropriate choice from `ERR_CODE`
  virtual ERR_CODE delete_child(const std::string &name) = 0;

  /// @brief Return the number of children in this branch.
  ///
  /// @return A pair containing a suitable error code and the number of children of this branch. The number of children
  ///         is only valid if the error code is ERR_CODE::NO_ERROR.
  virtual std::pair<ERR_CODE, uint64_t> num_children() = 0;

  /// @brief Enumerate immediate children of this branch.
  ///
  /// Return a list of children of this branch, ordered in the same order given natively by std::string.
  ///
  /// @param start_from The name of the first child to begin enumerating from. If this is empty then the first child is
  ///                   used as the beginning of the list. If the name doesn't exist, then the first child after this
  ///                   name is returned.
  ///
  /// @param max_count The maximum number of entries to return. If zero, there is no limit - this may have a
  ///                  significant performance impact for branches with many children!
  ///
  /// @return A pair containing a suitable error code and, if no error was encountered, a vector of names of children
  ///         in this branch.
  virtual std::pair<ERR_CODE, std::vector<std::string>> enum_children(std::string start_from, uint64_t max_count) = 0;

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
  ///
  /// @param[in] split_from_end If false, this will split the name at the first \ character. If True, it will split at
  ///                           the last \ character, which may be useful if trying to derive the name of the deepest
  ///                           child in the path name.
  void split_name(const std::string name_to_split,
                  std::string &first_part,
                  std::string &second_part,
                  bool split_from_end = false) const
  {
    uint64_t split_pos;

    if (split_from_end)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Split from end\n");
      split_pos = name_to_split.find_last_of("\\");
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Split from beginning\n");
      split_pos = name_to_split.find("\\");
    }

    if (split_pos == std::string::npos)
    {
      first_part = name_to_split;
      second_part = "";
    }
    else
    {
      first_part = name_to_split.substr(0, split_pos);
      second_part = name_to_split.substr(split_pos + 1, std::string::npos);
    }
  }
};
