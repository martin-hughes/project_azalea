/// @file
/// @brief A

#pragma once

class task_thread;

#include "klib/data_structures/lists.h"
#include "klib/synch/kernel_locks.h"

/// @brief A simple class that threads can wait on until it is triggered.
///
/// This can be extended to allow threads to wait for, for example, mutexes or other threads.
class WaitObject
{
public:
  WaitObject();
  virtual ~WaitObject();

  virtual void wait_for_signal();
  virtual void cancel_waiting_thread(task_thread *thread);

  virtual uint64_t threads_waiting();

protected:
  void trigger_next_thread();
  void trigger_all_threads();

  klib_list<task_thread *> _waiting_threads;
  kernel_spinlock _list_lock;
};