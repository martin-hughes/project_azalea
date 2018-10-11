#include "test/test_core/test.h"
#include "system_tree/system_tree_simple_branch.h"
#include "system_tree/system_tree.h"

#include "gtest/gtest.h"

using namespace std;

// A very simple test of the System tree. Initialise it, then play with some children.
TEST(SystemTreeTest, SimpleBranches)
{
  shared_ptr<system_tree_simple_branch> a = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> b = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> c = make_shared<system_tree_simple_branch>();

  shared_ptr<ISystemTreeBranch> ptr;
  shared_ptr<ISystemTreeLeaf> leaf_ptr;

  system_tree_init();

  ASSERT_EQ(system_tree()->add_child("", a), ERR_CODE::INVALID_NAME);
  ASSERT_EQ(system_tree()->add_child("branch_a", a), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("branch_b", b), ERR_CODE::NO_ERROR);

  ASSERT_EQ(system_tree()->add_child("branch_a", c), ERR_CODE::ALREADY_EXISTS);

  ASSERT_EQ(system_tree()->delete_child("branch_a"), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("branch_a", c), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->rename_child("branch_a", "branch_c"), ERR_CODE::NO_ERROR);

  ASSERT_EQ(system_tree()->get_child("branch_c", leaf_ptr), ERR_CODE::NO_ERROR);
  ASSERT_EQ(leaf_ptr, c);

  ASSERT_EQ(system_tree()->add_child("branch_c", a), ERR_CODE::ALREADY_EXISTS);
  ASSERT_EQ(system_tree()->delete_child("branch_c"), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->delete_child("branch_b"), ERR_CODE::NO_ERROR);

  test_only_reset_system_tree();
}
