// Tests the creation and destruction of processes.

#include "object_mgr/handles.h"
#include "object_mgr/object_mgr.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "test/test_core/test.h"

#include "gtest/gtest.h"

namespace
{
  void proc_tests_fake_task();
};

TEST(SchedulerTest, ProcessStartOneThreadAndExitThread)
{
  hm_gen_init();
  om_gen_init();
  task_gen_init();

  task_process *new_proc = task_create_new_process(proc_tests_fake_task);
  task_thread *child_thread;

  ASSERT_NE(new_proc, nullptr);
  ASSERT_NE(new_proc->child_threads.head, nullptr);
  ASSERT_NE(new_proc->child_threads.head->item, nullptr);

  child_thread = new_proc->child_threads.head->item;

  task_start_process(new_proc);

  task_destroy_thread(child_thread);

  test_only_reset_om();
  test_only_reset_task_mgr();
}

TEST(SchedulerTest, ProcessStartOneThreadAndExitProcess)
{
  hm_gen_init();
  om_gen_init();
  task_gen_init();

  task_process *new_proc = task_create_new_process(proc_tests_fake_task);
  task_thread *child_thread;

  ASSERT_NE(new_proc, nullptr);
  ASSERT_NE(new_proc->child_threads.head, nullptr);
  ASSERT_NE(new_proc->child_threads.head->item, nullptr);

  child_thread = new_proc->child_threads.head->item;

  task_start_process(new_proc);

  task_destroy_process(new_proc);

  test_only_reset_om();
  test_only_reset_task_mgr();
}


namespace
{
  void proc_tests_fake_task()
  {

  }
}