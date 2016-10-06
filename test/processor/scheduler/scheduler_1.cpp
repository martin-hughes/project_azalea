#include "test/processor/scheduler/scheduler_tests.h"
#include "object_mgr/handles.h"
#include "object_mgr/object_mgr.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include <iostream>

#include "test/test_core/test.h"

using namespace std;

void test_1_fake_task();

// Create a new list, add and delete items, check the list is still valid.
void scheduler_test_1()
{
  task_thread *thread_a;
  task_thread *idle_thread_a;
  task_process *proc_b;
  task_thread *thread_b;
  task_thread *expected_next;
  task_thread *ret_thread;
  int i;

  cout << "Scheduler task cycle tests" << endl;

  hm_gen_init();
  om_gen_init();
  task_gen_init(test_1_fake_task);

  // At the moment, there is only one thread, so it should be returned to us repeatedly.
  thread_a = task_get_next_thread();
  cout << "Thread A: " << thread_a << endl;
  ASSERT(thread_a != nullptr);
  for (i = 0; i < 10; i++);
  {
    ASSERT(thread_a == task_get_next_thread());
  }

  // Set it to not permit running. Ensure we get a different thread - should be the idle thread.
  thread_a->permit_running = false;

  idle_thread_a = task_get_next_thread();
  cout << "Idle thread: " << idle_thread_a << endl;
  ASSERT(thread_a != idle_thread_a);

  // Now, we should get the idle thread repeatedly.
  for (i = 0; i < 10; i++)
  {
    ASSERT(idle_thread_a = task_get_next_thread());
  }

  // Permit the first thread to run again, now we should get that repeatedly.
  thread_a->permit_running = true;
  for (i = 0; i < 10; i++);
  {
    ASSERT(thread_a == task_get_next_thread());
  }

  proc_b = task_create_new_process(test_1_fake_task, true);
  thread_b = (task_thread *)proc_b->child_threads.head->item;
  cout << "Thread B: " << thread_b << endl;
  task_start_process(proc_b);

  ret_thread = task_get_next_thread();
  cout << "Alternation test: received " << ret_thread << endl;
  ASSERT((ret_thread == thread_b) || (ret_thread == thread_a));

  for (i = 0; i < 10; i++)
  {
    expected_next = (ret_thread == thread_a) ? thread_b : thread_a;
    ret_thread = task_get_next_thread();
    cout << "Expected: " << expected_next << ", Got: " << ret_thread << endl;
    ASSERT(expected_next == ret_thread);
  }

  // Stop thread B, and check we only get thread A.
  thread_b->permit_running = false;
  for (int i = 0; i < 10; i++)
  {
    ASSERT(thread_a == task_get_next_thread());
  }

  // Switch to thread A, and check the same.
  thread_a->permit_running = false;
  thread_b->permit_running = true;
  for (int i = 0; i < 10; i++)
  {
    ASSERT(thread_b == task_get_next_thread());
  }

  // Disable both, and check we get the idle process
  thread_a->permit_running = false;
  thread_b->permit_running = false;
  for (int i = 0; i < 10; i++)
  {
    ASSERT(idle_thread_a == task_get_next_thread());
  }

  test_only_reset_om();

}


void test_1_fake_task()
{

}
