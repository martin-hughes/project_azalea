/// @file
/// @brief Implement `system_tree_simple_branch`, which can be used as a simple branch in system tree.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/system_tree_simple_branch.h"

system_tree_simple_branch::system_tree_simple_branch()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

system_tree_simple_branch::~system_tree_simple_branch()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

ERR_CODE system_tree_simple_branch::get_child_type(const kl_string &name, CHILD_TYPE &type)
{
  KL_TRC_ENTRY;

  ERR_CODE ret_code = ERR_CODE::NO_ERROR;
  uint64_t split_pos;
  kl_string direct_child;
  kl_string lower_child;
  std::shared_ptr<ISystemTreeBranch> child_branch;

  split_pos = name.find("\\");

  if (name == "")
  {
    type = CHILD_TYPE::NOT_FOUND;
    ret_code = ERR_CODE::INVALID_NAME;
  }
  else if (split_pos != kl_string::npos)
  {
    direct_child = name.substr(0, split_pos);
    lower_child = name.substr(split_pos + 1, kl_string::npos);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for ", lower_child, " in ", direct_child, "\n");

    if (!this->child_branches.contains(direct_child))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child branch not found anyway\n");
      type = CHILD_TYPE::NOT_FOUND;
      ret_code = ERR_CODE::NOT_FOUND;
    }
    else
    {
      child_branch = this->child_branches.search(direct_child);
      ASSERT(child_branch != nullptr);

      ret_code = child_branch->get_child_type(lower_child, type);
    }
  }
  else if (this->child_branches.contains(name))
  {
    type = CHILD_TYPE::BRANCH;
  }
  else if (this->child_leaves.contains(name))
  {
    type = CHILD_TYPE::LEAF;
  }
  else
  {
    type = CHILD_TYPE::NOT_FOUND;
    ret_code = ERR_CODE::NOT_FOUND;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Name searched for: ", name, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Type: ", static_cast<uint32_t>(type), "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Error code: ", ret_code, "\n");
  KL_TRC_EXIT;

  return ret_code;
}

ERR_CODE system_tree_simple_branch::get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  KL_TRC_ENTRY;

  ERR_CODE ret_code = ERR_CODE::NO_ERROR;
  kl_string our_part;
  kl_string child_part;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking for branch with name ", name, "to store in ", *branch, "\n");

  this->split_name(name, our_part, child_part);

  if (!this->child_branches.contains(our_part))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Branch not found\n");
    ret_code = ERR_CODE::NOT_FOUND;
    branch = nullptr;
  }
  else
  {
    branch = this->child_branches.search(our_part);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", *branch, "\n");

    if (child_part != "")
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for child branch ", child_part, "\n");
      ret_code = branch->get_branch(child_part, branch);
    }
  }

  KL_TRC_EXIT;

  return ret_code;
}

ERR_CODE system_tree_simple_branch::get_leaf(const kl_string &name, 
                                             std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  KL_TRC_ENTRY;

  ERR_CODE ret_code = ERR_CODE::NO_ERROR;
  kl_string our_part;
  kl_string child_part;
  std::shared_ptr<ISystemTreeBranch> branch;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking for leaf with name ", name, "to store in ", *leaf, "\n");

  this->split_name(name, our_part, child_part);

  if (child_part != "")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Passing to child branch ", our_part, "\n");

    if (this->child_branches.contains(our_part))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child branch found\n");
      branch = this->child_branches.search(our_part);
      ret_code = branch->get_leaf(child_part, leaf);
    }
    else
    {
      ret_code = ERR_CODE::NOT_FOUND;
      leaf = nullptr;
    }
  }
  else if (!this->child_leaves.contains(name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Leaf not found\n");
    ret_code = ERR_CODE::NOT_FOUND;
    leaf = nullptr;
  }
  else
  {
    leaf = this->child_leaves.search(name);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", *leaf, "\n");
  }

  KL_TRC_EXIT;

  return ret_code;
}

ERR_CODE system_tree_simple_branch::add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch)
{
  KL_TRC_ENTRY;

  CHILD_TYPE ct;
  ERR_CODE rt = ERR_CODE::NO_ERROR;
  uint64_t split_pos;
  kl_string next_branch_name;
  kl_string continuation_name;
  std::shared_ptr<ISystemTreeBranch> child_branch;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Adding branch with name ", name, " and address ", branch, "\n");

  split_pos = name.find("\\");

  if (branch == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't add null branch\n");
    rt = ERR_CODE::INVALID_PARAM;
  }
  else if (name == "")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can't add an unnamed child branch\n");
    rt = ERR_CODE::INVALID_NAME;
  }
  else if (split_pos != kl_string::npos)
  {
    next_branch_name = name.substr(0, split_pos);
    continuation_name = name.substr(split_pos + 1, kl_string::npos);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for ", continuation_name, " in ", next_branch_name, "\n");

    if (!this->child_branches.contains(next_branch_name))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child branch not found anyway\n");
      rt = ERR_CODE::NOT_FOUND;
    }
    else
    {
      child_branch = this->child_branches.search(next_branch_name);
      ASSERT(child_branch != nullptr);

      rt = child_branch->add_branch(continuation_name, branch);
    }
  }
  else if (this->get_child_type(name, ct) != ERR_CODE::NOT_FOUND)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "A child with that name already exists\n");
    rt = ERR_CODE::ALREADY_EXISTS;
  }
  else
  {
    this->child_branches.insert(name, branch);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", rt, "\n");

  KL_TRC_EXIT;

  return rt;
}

ERR_CODE system_tree_simple_branch::add_leaf (const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf)
{
  KL_TRC_ENTRY;

  CHILD_TYPE ct;
  ERR_CODE rt = ERR_CODE::NO_ERROR;
  uint64_t split_pos;
  kl_string next_branch_name;
  kl_string continuation_name;
  std::shared_ptr<ISystemTreeBranch> child_branch;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Adding leaf with name ", name, " and address ", leaf, "\n");

  split_pos = name.find("\\");

  if (leaf == nullptr)
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

    if (!this->child_branches.contains(next_branch_name))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child branch not found anyway\n");
      rt = ERR_CODE::NOT_FOUND;
    }
    else
    {
      child_branch = this->child_branches.search(next_branch_name);
      ASSERT(child_branch != nullptr);

      rt = child_branch->add_leaf(continuation_name, leaf);
    }
  }
  else if (this->get_child_type(name, ct) != ERR_CODE::NOT_FOUND)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "A child with that name already exists\n");
    rt = ERR_CODE::ALREADY_EXISTS;
  }
  else
  {
    this->child_leaves.insert(name, leaf);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", rt, "\n");

  KL_TRC_EXIT;

  return rt;
}

ERR_CODE system_tree_simple_branch::rename_child(const kl_string &old_name, const kl_string &new_name)
{
  KL_TRC_ENTRY;

  CHILD_TYPE ct;
  ERR_CODE rt = ERR_CODE::NO_ERROR;
  ERR_CODE intermediate;
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
    else if (!this->child_branches.contains(child_branch))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "The child branch does not exist\n");
      rt = ERR_CODE::NOT_FOUND;
    }
    else
    {
      grandchild_old_name = old_name.substr(old_dir_split + 1, kl_string::npos);
      grandchild_new_name = new_name.substr(new_dir_split + 1, kl_string::npos);

      b = this->child_branches.search(child_branch);
      ASSERT(b != nullptr);
      rt = b->rename_child(grandchild_old_name, grandchild_new_name);
    }
  }
  else if (old_dir_split != new_dir_split)
  {
    // As explained above, inter-branch moves are not supported.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Inter-branch moves not supported\n");
    rt = ERR_CODE::INVALID_OP;
  }
  else if (this->get_child_type(new_name, ct) != ERR_CODE::NOT_FOUND)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "New name already exists in children, can't duplicate\n");
    rt = ERR_CODE::ALREADY_EXISTS;
  }
  else
  {
    intermediate = this->get_child_type(old_name, ct);
    if (intermediate != ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Error ", static_cast<uint32_t>(intermediate),
                   " occurred while looking for child\n");
      rt = intermediate;
    }
    else
    {
      switch (ct)
      {
      case CHILD_TYPE::BRANCH:
        b = this->child_branches.search(old_name);
        this->child_branches.remove(old_name);
        this->child_branches.insert(new_name, b);
        break;

      case CHILD_TYPE::LEAF:
        l = this->child_leaves.search(old_name);
        this->child_leaves.remove(old_name);
        this->child_leaves.insert(new_name, l);
        break;

      default:
        ASSERT(0);
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", rt, "\n");
  KL_TRC_EXIT;

  return rt;
}

ERR_CODE system_tree_simple_branch::delete_child(const kl_string &name)
{
  KL_TRC_ENTRY;

  CHILD_TYPE ct;
  ERR_CODE rt = ERR_CODE::NO_ERROR;
  uint64_t split_pos;
  kl_string our_branch;
  kl_string grandchild;
  std::shared_ptr<ISystemTreeBranch> branch;

  if (this->get_child_type(name, ct) != ERR_CODE::NO_ERROR)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Child not found\n");
    rt = ERR_CODE::NOT_FOUND;
  }
  else
  {
    split_pos = name.find("\\");

    if (split_pos == kl_string::npos)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Deleting a direct child\n");

      switch (ct)
      {
      case CHILD_TYPE::BRANCH:
        this->child_branches.remove(name);
        break;

      case CHILD_TYPE::LEAF:
        this->child_leaves.remove(name);

        break;

      default:
        ASSERT(0);
      }

    }
    else
    {
      our_branch = name.substr(0, split_pos);
      grandchild = name.substr(split_pos + 1, kl_string::npos);

      branch = this->child_branches.search(our_branch);
      ASSERT(branch != nullptr);
      rt = branch->delete_child(grandchild);
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", rt, "\n");
  KL_TRC_EXIT;

  return rt;
}


ERR_CODE system_tree_simple_branch::create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  return ERR_CODE::INVALID_OP;
}

ERR_CODE system_tree_simple_branch::create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  kl_string first_part;
  kl_string second_part;
  ERR_CODE result;
  std::shared_ptr<ISystemTreeBranch> descendant;

  KL_TRC_ENTRY;

  split_name(name, first_part, second_part);
  if (second_part == "")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Create a leaf here\n");
    result = create_leaf_here(leaf);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Child leaf created, add here\n");
      result = this->add_leaf(name, leaf);

      // The call to add_leaf() has acquired a second reference to the leaf. One reference is owned by the object
      // manager. The other is owned by the creator of the leaf (i.e. the calling function)
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Looking for child branch\n");
    result = get_branch(first_part, descendant);

    if (result == ERR_CODE::NO_ERROR)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Pass request to child\n");
      result = descendant->create_leaf(second_part, leaf);
    }

  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE system_tree_simple_branch::create_leaf_here(std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  return ERR_CODE::INVALID_OP;
}
