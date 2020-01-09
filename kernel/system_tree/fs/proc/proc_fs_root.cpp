/// @file
/// @brief Implementation of the root of a file system similar to 'proc' in Linux.

//#define ENABLE_TRACING

#include <string>

#include "klib/klib.h"
#include "system_tree/fs/proc/proc_fs.h"

#include <stdio.h>

using namespace std;

proc_fs_root_branch::proc_fs_root_branch() :
  _zero_proxy(nullptr)
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

proc_fs_root_branch::~proc_fs_root_branch()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
}

//
// System Tree Branch interface.
//
ERR_CODE proc_fs_root_branch::add_child(const std::string &name, std::shared_ptr<ISystemTreeLeaf> child)
{
  std::string first_part;
  std::string second_part;
  std::shared_ptr<ISystemTreeBranch> child_branch;

  ERR_CODE result;
  KL_TRC_ENTRY;

  split_name(name, first_part, second_part);

  if (second_part != "")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to add child to existing branch\n");

    if (!map_contains(this->children, first_part))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child branch not found anyway\n");
      result = ERR_CODE::NOT_FOUND;
    }
    else
    {
      child_branch = std::dynamic_pointer_cast<ISystemTreeBranch>(this->children.find(first_part)->second);
      if (child_branch != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Try to add child to branch\n");
        result = child_branch->add_child(second_part, child);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Can't add a child here.\n");
        result = ERR_CODE::INVALID_OP;
      }
    }
  }
  else
  {
    // The only way to add an extra branch is to create a new process.
    result = ERR_CODE::INVALID_OP;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;

}

ERR_CODE proc_fs_root_branch::rename_child(const std::string &old_name, const std::string &new_name)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  // The branches and leafs of the proc FS have fixed names, they cannot be renamed.
  return ERR_CODE::INVALID_OP;
}

ERR_CODE proc_fs_root_branch::delete_child(const std::string &name)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  // The children of the proc FS can only be deleted by destroying the relevant process.
  return ERR_CODE::INVALID_OP;
}

//
// Interface from task manager.
//

/// @brief Add a process to the ones known about by the proc FS.
///
/// @param new_process The process to add.
///
/// @return A suitable error code.
ERR_CODE proc_fs_root_branch::add_process(std::shared_ptr<task_process> new_process)
{
  KL_TRC_ENTRY;

  ERR_CODE ec = ERR_CODE::NO_ERROR;
  std::string branch_name;
  char name_buffer[22];

  if (!new_process)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't add nullptr process\n");
    ec = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    // Since the Zero proxy branch requires a reference to the parent root branch, we can't easily create it in the
    // constructor - so construct it here if required.
    if (!this->_zero_proxy)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Zero proxy branch doesn't yet exist - create it\n");

      this->_zero_proxy = std::make_shared<proc_fs_zero_proxy_branch>(shared_from_this());
      system_tree_simple_branch::add_child("0", std::dynamic_pointer_cast<ISystemTreeBranch>(this->_zero_proxy));
    }
    snprintf(name_buffer, 22, "%p", new_process.get());
    branch_name = name_buffer;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Adding branch: ", branch_name, "\n");

    // This statement creates a new branch and adds it to the tree of branches we know about. Since these branches are
    // reference counted we need to consider that - the reference generated by the creation of the branch is owned by
    // the _proc_to_branch_map tree.
    std::shared_ptr<proc_fs_proc_branch> proc_ptr = proc_fs_proc_branch::create(new_process);
    system_tree_simple_branch::add_child(branch_name, proc_ptr);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", ec, "\n");
  KL_TRC_EXIT;

  return ec;
}

/// @brief Remove a process from the ones known about by the proc FS.
///
/// @param old_process The process to remove.
///
/// @return A suitable error code.
ERR_CODE proc_fs_root_branch::remove_process(std::shared_ptr<task_process> old_process)
{
  KL_TRC_ENTRY;

  ERR_CODE ec = ERR_CODE::NO_ERROR;
  std::shared_ptr<ISystemTreeLeaf> i_branch;
  std::shared_ptr<proc_fs_proc_branch> true_branch;
  std::string branch_name;
  char name_buffer[22];

  if (!old_process)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't remove nullptr process\n");
    ec = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    snprintf(name_buffer, 22, "%p", old_process.get());
    branch_name = name_buffer;
    ec = this->get_child(branch_name, i_branch);

    ASSERT(ec == ERR_CODE::NO_ERROR);

    true_branch = std::dynamic_pointer_cast<proc_fs_proc_branch>(i_branch);

    ASSERT(true_branch != nullptr);
    ASSERT(system_tree_simple_branch::delete_child(branch_name) == ERR_CODE::NO_ERROR);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", ec, "\n");
  KL_TRC_EXIT;

  return ec;
}