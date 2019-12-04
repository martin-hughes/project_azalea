/// @file
/// @brief Implement `system_tree_simple_branch`, which can be used as a simple branch in system tree.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/system_tree_simple_branch.h"

system_tree_simple_branch::system_tree_simple_branch()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_init(child_tree_lock);

  KL_TRC_EXIT;
}

system_tree_simple_branch::~system_tree_simple_branch()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

ERR_CODE system_tree_simple_branch::get_child(const kl_string &name,
                                              std::shared_ptr<ISystemTreeLeaf> &child)
{
  KL_TRC_ENTRY;

  ERR_CODE ret_code = ERR_CODE::NO_ERROR;
  kl_string our_part;
  kl_string child_part;
  std::shared_ptr<ISystemTreeBranch> branch;
  std::shared_ptr<ISystemTreeLeaf> direct_child;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking for child with name ", name, "to store in ", &child, "\n");

  this->split_name(name, our_part, child_part);

  klib_synch_spinlock_lock(child_tree_lock);
  if (children.contains(our_part))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieve direct child\n");
    direct_child = children.search(our_part);

    if (child_part != "")
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempt to pass request to child\n");

      branch = std::dynamic_pointer_cast<ISystemTreeBranch>(direct_child);

      if (branch)
      {
        ret_code = branch->get_child(child_part, child);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Direct child is not a branch\n");
        ret_code = ERR_CODE::NOT_FOUND;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found child ourselves\n");
      child = direct_child;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not a child of ours...\n");
    ret_code = ERR_CODE::NOT_FOUND;
  }
  klib_synch_spinlock_unlock(child_tree_lock);

  KL_TRC_EXIT;

  return ret_code;
}

ERR_CODE system_tree_simple_branch::add_child (const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child)
{
  KL_TRC_ENTRY;
  ERR_CODE rt = ERR_CODE::NO_ERROR;
  uint64_t split_pos;
  kl_string next_branch_name;
  kl_string continuation_name;
  std::shared_ptr<ISystemTreeBranch> child_branch;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Adding leaf with name ", name, " and address ", child.get(), "\n");
  split_pos = name.find("\\");

  klib_synch_spinlock_lock(child_tree_lock);
  if (child == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't add null leaf\n");
    rt = ERR_CODE::INVALID_PARAM;
  }
  else if (name == "")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't add an unnamed leaf\n");
    rt = ERR_CODE::INVALID_NAME;
  }
  else if (split_pos != kl_string::npos)
  {
    next_branch_name = name.substr(0, split_pos);
    continuation_name = name.substr(split_pos + 1, kl_string::npos);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for ", continuation_name, " in ", next_branch_name, "\n");

    if (!this->children.contains(next_branch_name))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child branch not found anyway\n");
      rt = ERR_CODE::NOT_FOUND;
    }
    else
    {
      child_branch = std::dynamic_pointer_cast<ISystemTreeBranch>(this->children.search(next_branch_name));
      if (child_branch != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Try to add child to branch\n");
        rt = child_branch->add_child(continuation_name, child);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Can't add a child here.\n");
        rt = ERR_CODE::INVALID_OP;
      }
    }
  }
  else if (children.contains(name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Already got a child of that name\n");
    rt = ERR_CODE::ALREADY_EXISTS;
  }
  else
  {
    this->children.insert(name, child);
  }
  klib_synch_spinlock_unlock(child_tree_lock);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", rt, "\n");

  KL_TRC_EXIT;

  return rt;
}

ERR_CODE system_tree_simple_branch::create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  kl_string first_part;
  kl_string second_part;
  ERR_CODE result;
  std::shared_ptr<ISystemTreeLeaf> direct_child;
  std::shared_ptr<ISystemTreeBranch> descendant;

  KL_TRC_ENTRY;

  split_name(name, first_part, second_part);
  if (second_part == "")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Create a leaf here\n");
    result = create_child_here(child);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child leaf created, add here\n");
      result = this->add_child(name, child);
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for child branch\n");
    result = get_child(first_part, direct_child);

    if (result == ERR_CODE::NO_ERROR)
    {
      descendant = std::dynamic_pointer_cast<ISystemTreeBranch>(direct_child);
      if (descendant)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Pass request to child\n");
        result = descendant->create_child(second_part, child);
      }
      else
      {
        result = ERR_CODE::INVALID_OP;
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE system_tree_simple_branch::rename_child(const kl_string &old_name, const kl_string &new_name)
{
  KL_TRC_ENTRY;
  ERR_CODE rt = ERR_CODE::NO_ERROR;
  std::shared_ptr<ISystemTreeBranch> b;
  std::shared_ptr<ISystemTreeLeaf> l;
  uint64_t old_dir_split;
  uint64_t new_dir_split;
  kl_string child_branch;
  kl_string grandchild_old_name;
  kl_string grandchild_new_name;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Renaming leaf with name ", old_name, " to ", &new_name, "\n");

  old_dir_split = old_name.find("\\");
  new_dir_split = new_name.find("\\");

  klib_synch_spinlock_lock(child_tree_lock);
  if ((old_dir_split != kl_string::npos) && (old_dir_split == new_dir_split))
  {
    child_branch = old_name.substr(0, old_dir_split);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Working a subdirectory: ", child_branch, "\n");

    // We don't support moving anything between two different branches. This prevents (e.g.) files being moved into the
    // part of the tree for devices. Other types of branch object are free to support inter-child-branch moves of
    // course.
    if (new_name.substr(0, old_dir_split) != child_branch)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Can't move between two different child branches\n");
      rt = ERR_CODE::INVALID_OP;
    }
    else if (!this->children.contains(child_branch))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "The child does not exist\n");
      rt = ERR_CODE::NOT_FOUND;
    }
    else
    {
      grandchild_old_name = old_name.substr(old_dir_split + 1, kl_string::npos);
      grandchild_new_name = new_name.substr(new_dir_split + 1, kl_string::npos);

      b = get_child_branch(child_branch);
      if (b != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found child branch\n");
        rt = b->rename_child(grandchild_old_name, grandchild_new_name);
      }
      else
      {
        rt = ERR_CODE::NOT_FOUND;
      }
    }
  }
  else if (old_dir_split != new_dir_split)
  {
    // As explained above, inter-branch moves are not supported.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Inter-branch moves not supported\n");
    rt = ERR_CODE::INVALID_OP;
  }
  else
  {
    if (children.contains(old_name))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Doing the rename\n");
      l = this->children.search(old_name);
      this->children.remove(old_name);
      this->children.insert(new_name, l);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not found!\n");
      rt = ERR_CODE::NOT_FOUND;
    }
  }
  klib_synch_spinlock_unlock(child_tree_lock);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", rt, "\n");
  KL_TRC_EXIT;

  return rt;
}

ERR_CODE system_tree_simple_branch::delete_child(const kl_string &name)
{
  KL_TRC_ENTRY;
  ERR_CODE rt = ERR_CODE::NO_ERROR;
  uint64_t split_pos;
  kl_string our_branch;
  kl_string grandchild;
  std::shared_ptr<ISystemTreeBranch> branch;

  split_pos = name.find("\\");

  klib_synch_spinlock_lock(child_tree_lock);
  if (split_pos == kl_string::npos)
  {
    if (children.contains(name))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Deleting a direct child\n");
      children.remove(name);
      rt = ERR_CODE::NO_ERROR;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Not found\n");
      rt = ERR_CODE::NOT_FOUND;
    }
  }
  else
  {
    our_branch = name.substr(0, split_pos);
    grandchild = name.substr(split_pos + 1, kl_string::npos);

    branch = this->get_child_branch(our_branch);
    if (branch != nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Submit to child branch");
      rt = branch->delete_child(grandchild);
    }
    else
    {
      rt = ERR_CODE::NOT_FOUND;
    }
  }
  klib_synch_spinlock_unlock(child_tree_lock);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", rt, "\n");
  KL_TRC_EXIT;

  return rt;
}

ERR_CODE system_tree_simple_branch::create_child_here(std::shared_ptr<ISystemTreeLeaf> &child)
{
  return ERR_CODE::INVALID_OP;
}

std::shared_ptr<ISystemTreeBranch> system_tree_simple_branch::get_child_branch(const kl_string &name)
{
  std::shared_ptr<ISystemTreeBranch> child;
  std::shared_ptr<ISystemTreeLeaf> direct_child;
  KL_TRC_ENTRY;

  // Don't lock here. This function should only be called internally, and thus should be lock-aware already.
  if (children.contains(name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieve child object\n");
    direct_child = children.search(name);

    child = std::dynamic_pointer_cast<ISystemTreeBranch>(direct_child);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", child.get(), "\n");
  KL_TRC_EXIT;

  return child;
}