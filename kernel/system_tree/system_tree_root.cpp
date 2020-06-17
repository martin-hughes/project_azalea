/// @file
/// @brief Implement `system_tree_root`, which handles the very root of the System Tree.

#include "types/system_tree_root.h"
#include "panic.h"
#include "k_assert.h"

uint32_t system_tree_root::number_of_instances = 0;

system_tree_root::system_tree_root()
{
  KL_TRC_ENTRY;

  ASSERT(system_tree_root::number_of_instances == 0);
  system_tree_root::number_of_instances++;

  root = std::make_shared<system_tree_simple_branch>();

  KL_TRC_EXIT;
}

system_tree_root::~system_tree_root()
{
  KL_TRC_ENTRY;

  // In some ways it'd be better if this destructor simply panic'd - but that'd confuse the test scripts. We need to be
  // able to destroy the system tree in order to demonstrate that no memory is leaked.
  KL_TRC_TRACE(TRC_LVL::ERROR, "System tree is being destroyed! Shouldn't occur\n");
  system_tree_root::number_of_instances--;

  KL_TRC_EXIT;
}

ERR_CODE system_tree_root::get_child(const std::string &name, std::shared_ptr<IHandledObject> &child)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Look for name: ", name, "\n");

  if (name[0] != '\\')
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Incomplete path\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else if (name == "\\")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Return root element\n");
    result = ERR_CODE::NO_ERROR;
    child = root;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Find in rest of tree\n");
    std::string remainder = name.substr(1);
    result = root->get_child(remainder, child);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE system_tree_root::add_child(const std::string &name, std::shared_ptr<IHandledObject> child)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  if (name[0] != '\\')
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Incomplete path\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Add somewhere in rest of tree\n");
    std::string remainder = name.substr(1);
    result = root->add_child(remainder, child);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE system_tree_root::create_child(const std::string &name, std::shared_ptr<IHandledObject> &child)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  if (name[0] != '\\')
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Incomplete path\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Find in rest of tree\n");
    std::string remainder = name.substr(1);
    result = root->create_child(remainder, child);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE system_tree_root::rename_child(const std::string &old_name, const std::string &new_name)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  if ((old_name[0] != '\\') || (new_name[0] != '\\'))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Incomplete path\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Rename in rest of tree\n");
    std::string old_remainder = old_name.substr(1);
    std::string new_remainder = new_name.substr(1);
    result = root->rename_child(old_remainder, new_remainder);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE system_tree_root::delete_child(const std::string &name)
{
  ERR_CODE result;
  KL_TRC_ENTRY;

  if (name[0] != '\\')
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Incomplete path\n");
    result = ERR_CODE::NOT_FOUND;
  }
  else if (name == "\\")
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Delete root element??\n");
    result = ERR_CODE::INVALID_OP;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Find in rest of tree\n");
    std::string remainder = name.substr(1);
    result = root->delete_child(remainder);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

std::pair<ERR_CODE, uint64_t> system_tree_root::num_children()
{
  return root->num_children();
}

std::pair<ERR_CODE, std::vector<std::string>>
    system_tree_root::enum_children(std::string start_from, uint64_t max_count)
{
  return root->enum_children(start_from, max_count);
}
