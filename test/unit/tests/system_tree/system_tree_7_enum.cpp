#include "test_core/test.h"
#include "types/system_tree_simple_branch.h"
#include "system_tree.h"
#include "azalea/syscall.h"
#include "processor.h"

#include "gtest/gtest.h"

using namespace std;

// A very simple test of the System tree. Initialise it, then play with some children.
TEST(SystemTreeTest, SimpleEnums)
{
  shared_ptr<system_tree_simple_branch> a = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> b = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> c = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> d = make_shared<system_tree_simple_branch>();

  shared_ptr<ISystemTreeBranch> ptr;
  shared_ptr<IHandledObject> leaf_ptr;

  system_tree_init();

  ASSERT_EQ(system_tree()->add_child("\\branch_a", a), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("\\branch_b", b), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("\\branch_c", c), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("\\branch_d", d), ERR_CODE::NO_ERROR);

  auto p = std::make_pair<ERR_CODE, uint64_t>(ERR_CODE::NO_ERROR, 4);
  ASSERT_EQ(system_tree()->num_children(), p );

  auto results = system_tree()->enum_children("", 0);
  ASSERT_EQ(results.second[0], "branch_a");
  ASSERT_EQ(results.second[1], "branch_b");
  ASSERT_EQ(results.second[2], "branch_c");
  ASSERT_EQ(results.second[3], "branch_d");
  ASSERT_EQ(results.second.size(), 4);

  // Delete a branch and check it's reflected in the results.
  ASSERT_EQ(system_tree()->delete_child("\\branch_c"), ERR_CODE::NO_ERROR);

  auto q = std::make_pair<ERR_CODE, uint64_t>(ERR_CODE::NO_ERROR, 3);
  ASSERT_EQ(system_tree()->num_children(), q );

  results = system_tree()->enum_children("", 0);
  ASSERT_EQ(results.second[0], "branch_a");
  ASSERT_EQ(results.second[1], "branch_b");
  ASSERT_EQ(results.second[2], "branch_d");
  ASSERT_EQ(results.second.size(), 3);

  // Check that the alphabetical start_from is correct, first on a non-existent name within the bunch
  results = system_tree()->enum_children("branch_c", 0);
  ASSERT_EQ(results.second[0], "branch_d");
  ASSERT_EQ(results.second.size(), 1);

  // Then on a name that exists in the bunch:
  results = system_tree()->enum_children("branch_b", 0);
  ASSERT_EQ(results.second[0], "branch_b");
  ASSERT_EQ(results.second[1], "branch_d");
  ASSERT_EQ(results.second.size(), 2);

  // Then on a name after the bunch
  results = system_tree()->enum_children("definitely_After", 0);
  ASSERT_EQ(results.second.size(), 0);

  // And on a name before the whole bunch
  results = system_tree()->enum_children("aaaa", 0);
  ASSERT_EQ(results.second[0], "branch_a");
  ASSERT_EQ(results.second[1], "branch_b");
  ASSERT_EQ(results.second[2], "branch_d");
  ASSERT_EQ(results.second.size(), 3);

  // Finally check that the maximum numbers work OK.
  results = system_tree()->enum_children("", 2);
  ASSERT_EQ(results.second[0], "branch_a");
  ASSERT_EQ(results.second[1], "branch_b");
  ASSERT_EQ(results.second.size(), 2);

  results = system_tree()->enum_children("", 5);
  ASSERT_EQ(results.second[0], "branch_a");
  ASSERT_EQ(results.second[1], "branch_b");
  ASSERT_EQ(results.second[2], "branch_d");
  ASSERT_EQ(results.second.size(), 3);

  test_only_reset_system_tree();
}

TEST(SystemTreeTest, SyscallEnums)
{
  shared_ptr<task_process> sys_proc;

  shared_ptr<system_tree_simple_branch> a = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> b = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> c = make_shared<system_tree_simple_branch>();
  shared_ptr<system_tree_simple_branch> d = make_shared<system_tree_simple_branch>();
  shared_ptr<ISystemTreeBranch> ptr;
  shared_ptr<IHandledObject> leaf_ptr;

  std::unique_ptr<char[]> buf = std::unique_ptr<char[]>(new char[59]);
  const uint64_t buf_size = 59;
  uint64_t variable_size{buf_size};
  GEN_HANDLE a_handle;
  char **ptr_table = reinterpret_cast<char **>(buf.get());

  system_tree_init();
  sys_proc = task_init();

  test_only_set_cur_thread(sys_proc->child_threads.head->item.get());

  ASSERT_EQ(system_tree()->add_child("\\a", a), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("\\a\\branch_b", b), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("\\a\\branch_c", c), ERR_CODE::NO_ERROR);
  ASSERT_EQ(system_tree()->add_child("\\a\\branch_d", d), ERR_CODE::NO_ERROR);

  ASSERT_EQ(az_open_handle("\\a", 2, &a_handle, 0), ERR_CODE::NO_ERROR);

  // Try to request an enum for a branch that doesn't exist.
  ASSERT_EQ(az_enum_children(a_handle + 1, nullptr, 0, 0, buf.get(), &variable_size), ERR_CODE::NOT_FOUND);

  // Feed a null buffer and see what size comes back.
  variable_size = 0;
  ASSERT_EQ(az_enum_children(a_handle, nullptr, 0, 0, nullptr, &variable_size), ERR_CODE::NO_ERROR);
  ASSERT_EQ(variable_size, buf_size);

  // Feed it the complete buffer and see that it gets filled successfully.
  variable_size = buf_size;
  ASSERT_EQ(az_enum_children(a_handle, nullptr, 0, 0, buf.get(), &variable_size), ERR_CODE::NO_ERROR);
  ASSERT_EQ(variable_size, buf_size);
  ASSERT_STREQ(ptr_table[0], "branch_b");
  ASSERT_STREQ(ptr_table[1], "branch_c");
  ASSERT_STREQ(ptr_table[2], "branch_d");
  ASSERT_EQ(ptr_table[3], nullptr);

  // Feed it a smaller buffer and check the results are truncated correctly.
  variable_size = buf_size - 1;
  ASSERT_EQ(az_enum_children(a_handle, nullptr, 0, 0, buf.get(), &variable_size), ERR_CODE::NO_ERROR);
  ASSERT_EQ(variable_size, buf_size);
  ASSERT_STREQ(ptr_table[0], "branch_b");
  ASSERT_STREQ(ptr_table[1], "branch_c");
  ASSERT_EQ(ptr_table[2], nullptr);

  // Restrict results to one entry, and check that it is correct.
  variable_size = buf_size;
  ASSERT_EQ(az_enum_children(a_handle, nullptr, 0, 1, buf.get(), &variable_size), ERR_CODE::NO_ERROR);
  ASSERT_EQ(variable_size, 25);
  ASSERT_STREQ(ptr_table[0], "branch_b");
  ASSERT_EQ(ptr_table[1], nullptr);

  // Check "start from" works correctly.
  variable_size = buf_size;
  ASSERT_EQ(az_enum_children(a_handle, "branch_c", 8, 0, buf.get(), &variable_size), ERR_CODE::NO_ERROR);
  ASSERT_EQ(variable_size, buf_size - 17);
  ASSERT_STREQ(ptr_table[0], "branch_c");
  ASSERT_STREQ(ptr_table[1], "branch_d");
  ASSERT_EQ(ptr_table[2], nullptr);

  test_only_set_cur_thread(nullptr);
  test_only_reset_task_mgr();
  test_only_reset_system_tree();
}
