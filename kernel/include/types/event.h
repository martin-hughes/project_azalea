/// @file
/// @brief An event style IPC object.

#pragma once

#include "ipc_core.h"
#include "handled_obj.h"

#include "types/spinlock.h"
#include "types/list.h"

class task_thread;

namespace ipc
{
  /// @brief An object that
  class event : public virtual IHandledObject
  {
  public:
    event();
    event(bool auto_reset);
    event(bool auto_reset, ipc::raw_spinlock &lock_override);
    event(event &) = delete;
    event(event &&) = delete;
    virtual ~event();

    event &operator=(const event &) = delete;
    event &operator=(const event &&) = delete;

    virtual void wait();
    virtual bool timed_wait(uint64_t wait_in_us);

    virtual void reset();

    virtual bool should_still_sleep();
    virtual void before_wake_cb();

    virtual void signal_event();
    virtual bool cancel_waiting_thread(task_thread *thread);

  protected:
    klib_list<task_thread *> _waiting_threads; ///< List of threads waiting for this WaitObject to be signalled.
    ipc::raw_spinlock &_list_lock; ///< Lock protecting _waiting_threads.
    ipc::raw_spinlock internal_lock; ///< Lock to use for _list_lock if no other lock provided.

    bool triggered{false}; ///< Is this event in the triggered state?
    bool auto_reset{false};

    virtual void trigger_next_thread(const bool should_lock);
  };
};

#include "types/thread.h" // Required to ensure the complete declaration of task_thread.
