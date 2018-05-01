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
  task_thread *thread_a;
  task_thread *idle_thread_a;
  task_thread *ret_thread;
  TestWaitObject wait_obj;
  int i;

  hm_gen_init();
  om_gen_init();
  task_gen_init(test_fake_task);

  // task_gen_init adds a thread for the IRQ slowpath to the first process. Disable the first thread arbitrarily, since
  // the process starts with two.
  thread_a = task_get_next_thread();
  thread_a->permit_running = false;

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

  test_only_reset_om();
  test_only_reset_task_mgr();
}

void test_fake_task()
{

}
