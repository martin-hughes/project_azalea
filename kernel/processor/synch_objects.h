/// @file
/// @brief An object that can be waited on, which blocks execution until signalled.

#pragma once

class task_thread;

#include "klib/data_structures/lists.h"
#include "klib/synch/kernel_locks.h"
#include "klib/synch/kernel_mutexes.h"
#include "klib/synch/kernel_semaphores.h"
#include "object_mgr/handled_obj.h"

/// @brief A simple class that threads can wait on until it is triggered. It is pretty much a simple semaphore.
///
/// This can be extended to allow threads to wait for, for example, mutexes or other threads.
class WaitObject
{
public:
  WaitObject();
  virtual ~WaitObject();

  virtual void wait_for_signal(uint64_t max_wait);
  virtual void cancel_waiting_thread(task_thread *thread);

  virtual uint64_t threads_waiting();

  /// @brief Maximum possible time to wait for object to become signalled.
  static const uint64_t MAX_WAIT = 0xFFFFFFFFFFFFFFFF;

protected:
  virtual void trigger_next_thread(const bool should_lock = true);
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

  virtual void wait_for_signal(uint64_t max_wait) override;

protected:
  virtual void trigger_next_thread(const bool should_lock = true) override;
  virtual void trigger_all_threads() override;

  volatile bool already_triggered; ///< Has this wait object already had at least one thread be triggered?
};

/// @brief Wrapper around klib's mutex object to allow it to be exposed by the syscall API.
///
class syscall_mutex_obj : public WaitObject, public IHandledObject
{
public:
  syscall_mutex_obj();
  virtual ~syscall_mutex_obj();

  virtual void wait_for_signal(uint64_t max_wait) override;
  virtual bool release();

protected:
  virtual void trigger_next_thread(const bool should_lock = true) override;
  virtual void trigger_all_threads() override;

  klib_mutex base_mutex; ///< The underlying object within the kernel.
};

/// @brief Wrapper around klib's semaphore object to allow it to be exposed by the syscall API.
///
class syscall_semaphore_obj : public WaitObject, public IHandledObject
{
public:
  syscall_semaphore_obj(uint64_t max_users, uint64_t start_users);
  virtual ~syscall_semaphore_obj();

  virtual void wait_for_signal(uint64_t max_wait) override;
  virtual bool signal();

protected:
  virtual void trigger_next_thread(const bool should_lock = true) override;
  virtual void trigger_all_threads() override;

  klib_semaphore base_semaphore;
};
