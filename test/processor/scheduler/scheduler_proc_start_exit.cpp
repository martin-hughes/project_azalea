// Tests the creation and destruction of processes.

#include "object_mgr/handles.h"
#include "object_mgr/object_mgr.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "system_tree/system_tree.h"
#include "test/test_core/test.h"

#include "gtest/gtest.h"

using namespace std;

TEST(SchedulerTest, ProcessStartOneThreadAndExitThread)
{
  hm_gen_init();
  system_tree_init();
  task_gen_init();

  shared_ptr<task_process> new_proc = task_process::create(dummy_thread_fn);
  task_thread *child_thread;

  ASSERT_NE(new_proc, nullptr);
  ASSERT_NE(new_proc->child_threads.head, nullptr);
  ASSERT_NE(new_proc->child_threads.head->item, nullptr);

  child_thread = new_proc->child_threads.head->item.get();

  new_proc->start_process();

  child_thread->destroy_thread();

  test_only_reset_task_mgr();
  test_only_reset_system_tree();
}

TEST(SchedulerTest, ProcessStartOneThreadAndExitProcess)
{
  hm_gen_init();
  system_tree_init();
  task_gen_init();

  shared_ptr<task_process> new_proc = task_process::create(dummy_thread_fn);
  task_thread *child_thread;

  ASSERT_NE(new_proc, nullptr);
  ASSERT_NE(new_proc->child_threads.head, nullptr);
  ASSERT_NE(new_proc->child_threads.head->item, nullptr);

  child_thread = new_proc->child_threads.head->item.get();

  new_proc->start_process();

  new_proc->destroy_process();

  test_only_reset_task_mgr();
  test_only_reset_system_tree();
}