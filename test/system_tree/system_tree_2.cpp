#include "test/test_core/test.h"
#include "system_tree/system_tree_simple_branch.h"
#include "system_tree/system_tree.h"

#include "gtest/gtest.h"

using namespace std;

// Test the tree structure of System Tree
TEST(SystemTreeTest, SimpleTree)
{
  system_tree_root *root;
  shared_ptr<ISystemTreeBranch> branch;
  CHILD_TYPE ct;

  system_tree_init();
  root = system_tree();

  ASSERT_NE(root, nullptr);

  shared_ptr<system_tree_simple_branch> a = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> b = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> a_a = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> a_b = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> b_a = make_shared<system_tree_simple_branch>();

  ASSERT_EQ(root->add_branch("a", a), ERR_CODE::NO_ERROR);
  ASSERT_EQ(root->add_branch("b", b), ERR_CODE::NO_ERROR);
  ASSERT_EQ(a->add_branch("a", a_a), ERR_CODE::NO_ERROR);
  ASSERT_EQ(a->add_branch("b", a_b), ERR_CODE::NO_ERROR);
  ASSERT_EQ(b->add_branch("a", b_a), ERR_CODE::NO_ERROR);

  ASSERT_EQ(root->get_child_type("a\\a", ct), ERR_CODE::NO_ERROR);
  ASSERT_EQ(ct, CHILD_TYPE::BRANCH);

  ASSERT_EQ(root->get_child_type("a\\b", ct), ERR_CODE::NO_ERROR);
  ASSERT_EQ(ct, CHILD_TYPE::BRANCH);

  ASSERT_EQ(root->get_child_type("b\\a", ct), ERR_CODE::NO_ERROR);
  ASSERT_EQ(ct, CHILD_TYPE::BRANCH);

  ASSERT_EQ(root->get_branch("a\\b", branch), ERR_CODE::NO_ERROR);
  ASSERT_EQ(branch, a_b);

  ASSERT_EQ(root->rename_child("a\\a", "a\\c"), ERR_CODE::NO_ERROR);
  ASSERT_EQ(root->get_child_type("a\\c", ct), ERR_CODE::NO_ERROR);
  ASSERT_EQ(ct, CHILD_TYPE::BRANCH);
  ASSERT_EQ(root->get_branch("a\\c", branch), ERR_CODE::NO_ERROR);
  ASSERT_EQ(branch, a_a);

  ASSERT_EQ(root->rename_child("a\\b", "b\\b"), ERR_CODE::INVALID_OP);

  ASSERT_EQ(root->delete_child("b\\a"), ERR_CODE::NO_ERROR);
  ASSERT_EQ(root->get_child_type("b\\a", ct), ERR_CODE::NOT_FOUND);

  test_only_reset_system_tree();
}
