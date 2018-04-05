#include "system_tree/system_tree_simple_branch.h"
#include "system_tree/system_tree.h"

#include "gtest/gtest.h"

using namespace std;

// A very simple test of the System tree. Initialise it, then play with some children.
TEST(SystemTreeTest, SimpleBranches)
{
  system_tree_simple_branch a;
  system_tree_simple_branch b;
  system_tree_simple_branch c;

  ISystemTreeBranch *ptr;
  ISystemTreeLeaf *leaf_ptr;
  CHILD_TYPE ct;

  system_tree_init();

  ASSERT_EQ(system_tree()->add_branch("", &a), ERR_CODE::INVALID_NAME);
  ASSERT_EQ(system_tree()->add_branch("branch_a", &a), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_branch("branch_b", &b), ERR_CODE::NO_ERROR);

  ASSERT_EQ(system_tree()->add_branch("branch_a", &c), ERR_CODE::ALREADY_EXISTS);

  ASSERT_EQ(system_tree()->delete_child("branch_a"), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_branch("branch_a", &c), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->rename_child("branch_a", "branch_c"), ERR_CODE::NO_ERROR);

  ASSERT_EQ(system_tree()->get_child_type("branch_a", ct), ERR_CODE::NOT_FOUND);
  ASSERT_EQ(ct, CHILD_TYPE::NOT_FOUND);
  ASSERT_EQ(system_tree()->get_child_type("branch_c", ct), ERR_CODE::NO_ERROR);
  ASSERT_EQ(ct, CHILD_TYPE::BRANCH);

  ASSERT_EQ(system_tree()->get_branch("branch_c", &ptr), ERR_CODE::NO_ERROR);
  ASSERT_EQ(ptr, &c);

  ASSERT_EQ(system_tree()->get_leaf("branch_c", &leaf_ptr), ERR_CODE::NOT_FOUND);

  ASSERT_EQ(system_tree()->add_branch("branch_c", &a), ERR_CODE::ALREADY_EXISTS);
  ASSERT_EQ(system_tree()->delete_child("branch_c"), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->delete_child("branch_b"), ERR_CODE::NO_ERROR);

  ASSERT_EQ(system_tree()->get_child_type("branch_b", ct), ERR_CODE::NOT_FOUND);

  test_only_reset_system_tree();
}
