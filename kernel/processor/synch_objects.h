/// @file
/// @brief A

#pragma once

class task_thread;

#include "klib/data_structures/lists.h"
#include "klib/synch/kernel_locks.h"

/// @brief A simple class that threads can wait on until it is triggered. It is pretty much a simple semaphore.
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
  virtual void trigger_next_thread();
  virtual void trigger_all_threads();

  klib_list<task_thread *> _waiting_threads; ///< List of threads waiting for this WaitObject to be signalled.
  kernel_spinlock _list_lock; ///< Lock protecting _waiting_threads.
};

/// @brief This class is identical in operation to WaitObject, except it will only wait the first time.
///
/// Before trigger_next_thread() is called the first time, calls to wait_for_signal() will wait as expected. After the
/// first thread is triggered, calls to wait_for_signal() will simply return immediately.
class WaitForFirstTriggerObject : public WaitObject
{
public:
  WaitForFirstTriggerObject();
  virtual ~WaitForFirstTriggerObject();

  virtual void wait_for_signal() override;

protected:
  virtual void trigger_next_thread() override;

  bool already_triggered; ///< Has this wait object already had at least one thread be triggered?
};