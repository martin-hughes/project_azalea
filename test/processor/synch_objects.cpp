/// @file Basic tests of the kernel wait objects system.
///

#include "gtest/gtest.h"
#include "test/test_core/test.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/synch_objects.h"
#include "object_mgr/object_mgr.h"

class TestWaitObject : public WaitObject
{
public:
  ~TestWaitObject() { }
  void test_trigger() { this->trigger_next_thread(); }
};

void test_fake_task();

TEST(ProcessorTests, WaitObjects)
{
  task_process *sys_proc;
  task_process *proc_a;
  task_thread *thread_a;
  task_thread *idle_thread_a;
  task_thread *ret_thread;
  TestWaitObject wait_obj;
  int i;

  hm_gen_init();
  om_gen_init();
  sys_proc = task_init();

  // Don't run any threads from the system process, it just confuses the rest of the test.
  task_stop_process(sys_proc);

  proc_a = task_create_new_process(test_fake_task);
  task_start_process(proc_a);

  // At the moment, there is only one thread, so it should be returned to us repeatedly.
  thread_a = task_get_next_thread();
  ASSERT_NE(thread_a, nullptr);
  for (i = 0; i < 10; i++)
  {
    ASSERT_EQ(thread_a, task_get_next_thread());
  }

  // Make thread A wait for the wait object, then we should get an idle thread repeatedly.
  test_only_set_cur_thread(thread_a);
  wait_obj.wait_for_signal();

  idle_thread_a = task_get_next_thread();
  ASSERT_NE(thread_a, idle_thread_a);

  // Now, we should get the idle thread repeatedly.
  for (i = 0; i < 10; i++)
  {
    ASSERT_EQ(idle_thread_a, task_get_next_thread());
  }

  // Permit the first thread to run again, now we should get that repeatedly.
  wait_obj.cancel_waiting_thread(thread_a);
  for (i = 0; i < 10; i++)
  {
    ASSERT_EQ(thread_a, task_get_next_thread());
  }

  // Make thread A wait for the wait object, then we should get an idle thread repeatedly.
  test_only_set_cur_thread(thread_a);
  wait_obj.wait_for_signal();

  idle_thread_a = task_get_next_thread();
  ASSERT_NE(thread_a, idle_thread_a);

  // Now, we should get the idle thread repeatedly.
  for (i = 0; i < 10; i++)
  {
    ASSERT_EQ(idle_thread_a, task_get_next_thread());
  }

  // Signal the thread, then we should get that again repeatedly.
  wait_obj.test_trigger();
  for (i = 0; i < 10; i++)
  {
    ASSERT_EQ(thread_a, task_get_next_thread());
  }

  // Switch to having the idle thread be current. It is necessary to unschedule all tasks as otherwise
  // test_only_reset_task_mgr() gets stuck waiting for the thread to be unscheduled.
  task_stop_process(proc_a);
  task_get_next_thread();
  test_only_set_cur_thread(nullptr);
  task_destroy_process(proc_a);

  test_only_reset_om();
  test_only_reset_task_mgr();
}

void test_fake_task()
{

}
