// Dummy task manager library. Implements a select part of the task manager, for use in tests.

#include <pthread>
#include "processor/processor.h"

task_thread *task_create_new_thread(ENTRY_PROC entry_point, task_process *parent_process)
{
  asdf
}

task_thread *task_get_cur_thread()
{
  pthread_t s = pthread_self();

  // Mangle the type system!
  return (task_thread *)(s);
}

void task_stop_thread(task_thread *thread)
{
  pthread_t thread_to_stop = (pthread_t)thread;

  asdf
}

void task_start_thread(task_thread *thread)
{
  pthread_t thread_to_start = (pthread_t)thread;

  asdf
}

void task_continue_this_thread()
{
  sadf
}

void task_resume_scheduling()
{
  asfd
}

void task_yield()
{
  pthread_yield();
}
