// Tests the creation and destruction of processes.

#include "handles.h"
#include "object_mgr.h"
#include "processor.h"
#include "../processor/processor-int.h"
#include "system_tree.h"
#include "test_core/test.h"

#include "gtest/gtest.h"

using namespace std;

TEST(SchedulerTest, ProcessStartOneThreadAndExitThread)
{
  hm_gen_init();
  system_tree_init();
  task_gen_init();

  shared_ptr<task_process> new_proc = task_process::create(dummy_thread_fn);
  task_thread *child_thread;

  ASSERT_TRUE(new_proc != nullptr);
  ASSERT_TRUE(new_proc->child_threads.head != nullptr);
  ASSERT_TRUE(new_proc->child_threads.head->item != nullptr);

  child_thread = new_proc->child_threads.head->item.get();

  new_proc->start_process();

  child_thread->destroy_thread();

  child_thread = nullptr;
  new_proc = nullptr;

  test_only_reset_task_mgr();
  test_only_reset_system_tree();
  test_only_reset_allocator();
}

TEST(SchedulerTest, ProcessStartOneThreadAndExitProcess)
{
  hm_gen_init();
  system_tree_init();
  task_gen_init();

  shared_ptr<task_process> new_proc = task_process::create(dummy_thread_fn);
  task_thread *child_thread;

  ASSERT_TRUE(new_proc != nullptr);
  ASSERT_TRUE(new_proc->child_threads.head != nullptr);
  ASSERT_TRUE(new_proc->child_threads.head->item != nullptr);

  child_thread = new_proc->child_threads.head->item.get();

  new_proc->start_process();

  new_proc->destroy_process(0);

  new_proc = nullptr;
  child_thread = nullptr;

  test_only_reset_task_mgr();
  test_only_reset_system_tree();
  test_only_reset_allocator();
}
