#include "test/test_core/test.h"
#include "processor/processor.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/pipe/pipe_fs.h"
#include "test/test_core/test.h"

#include "gtest/gtest.h"

using namespace std;

// A simple test of the Proc FS objects within ST.
TEST(SystemTreeTest, ProcFsCreateAndDestroy)
{
  ERR_CODE ec;

  system_tree_init();
  task_gen_init();

  test_only_reset_task_mgr();
  test_only_reset_system_tree();
}

TEST(SystemTreeTest, ProcFsOneProcessIdFile)
{
  shared_ptr<ISystemTreeBranch> zero_branch;
  shared_ptr<ISystemTreeLeaf> id_leaf;
  shared_ptr<IBasicFile> id_file;
  ERR_CODE ec;
  char read_buffer[22];
  char expected_buffer[22];
  uint64_t br;

  system_tree_init();
  task_gen_init();

  shared_ptr<task_process> proc = task_process::create(dummy_thread_fn);
  ASSERT_NE(proc, nullptr);

  test_only_set_cur_thread(proc->child_threads.head->item.get());

  ec = system_tree()->get_branch("proc\\0", zero_branch);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);

  ec = zero_branch->get_leaf("id", id_leaf);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);

  id_file = dynamic_pointer_cast<IBasicFile>(id_leaf);
  ASSERT_NE(id_file, nullptr);

  memset(read_buffer, 0, 22);
  id_file->read_bytes(0, 22, reinterpret_cast<uint8_t *>(read_buffer), 22, br);

  snprintf(expected_buffer, 22, "%p", proc.get());

  ASSERT_EQ(strncmp(read_buffer, expected_buffer, 22), 0);

  test_only_set_cur_thread(nullptr);
  proc->destroy_process();

  test_only_reset_task_mgr();
  test_only_reset_system_tree();
}
