#include "test/system_tree/system_tree_test_list.h"
#include "test/test_core/test.h"
#include "system_tree/system_tree_simple_branch.h"
#include "system_tree/system_tree.h"
#include <iostream>

using namespace std;

// Test the tree structure of System Tree
void system_tree_test_2()
{
  system_tree_root *root;
  ISystemTreeBranch *branch;
  CHILD_TYPE ct;

  system_tree_init();
  root = system_tree();

  ASSERT(root != nullptr);

  system_tree_simple_branch a, b, a_a, a_b, b_a;

  ASSERT(root->add_branch("a", &a) == ERR_CODE::NO_ERROR);
  ASSERT(root->add_branch("b", &b) == ERR_CODE::NO_ERROR);
  ASSERT(a.add_branch("a", &a_a) == ERR_CODE::NO_ERROR);
  ASSERT(a.add_branch("b", &a_b) == ERR_CODE::NO_ERROR);
  ASSERT(b.add_branch("a", &b_a) == ERR_CODE::NO_ERROR);

  ASSERT(root->get_child_type("a\\a", ct) == ERR_CODE::NO_ERROR);
  ASSERT(ct == CHILD_TYPE::BRANCH);

  ASSERT(root->get_child_type("a\\b", ct) == ERR_CODE::NO_ERROR);
  ASSERT(ct == CHILD_TYPE::BRANCH);

  ASSERT(root->get_child_type("b\\a", ct) == ERR_CODE::NO_ERROR);
  ASSERT(ct == CHILD_TYPE::BRANCH);

  ASSERT(root->get_branch("a\\b", &branch) == ERR_CODE::NO_ERROR);
  ASSERT(branch = &a_b);

  ASSERT(root->rename_child("a\\a", "a\\c") == ERR_CODE::NO_ERROR);
  ASSERT(root->get_child_type("a\\c", ct) == ERR_CODE::NO_ERROR);
  ASSERT(ct == CHILD_TYPE::BRANCH);
  ASSERT(root->get_branch("a\\c", &branch) == ERR_CODE::NO_ERROR);
  ASSERT(branch = &a_a);

  ASSERT(root->rename_child("a\\b", "b\\b") == ERR_CODE::INVALID_OP);

  ASSERT(root->delete_child("b\\a") == ERR_CODE::NO_ERROR);
  ASSERT(root->get_child_type("b\\a", ct) == ERR_CODE::NOT_FOUND);

  test_only_reset_system_tree();
}