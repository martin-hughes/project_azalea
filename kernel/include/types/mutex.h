/// @file
/// @brief Kernel mutex object

#pragma once

#include "ipc_core.h"
#include "types/spinlock.h"
#include "handled_obj.h"
#include "types/list.h"
#include "types/thread.h"

namespace ipc
{
  class base_mutex
  {
  public:
    /// @brief Default constructor.
    ///
    /// Constructs a non-recursive mutex.
    constexpr base_mutex() noexcept : base_mutex{false} { };

    /// @brief Optional constructor.
    ///
    /// @param recursive Whether or not the mutex should be recursive.
    constexpr base_mutex(bool recursive) noexcept : recursive{recursive} { };

    base_mutex(base_mutex &) = delete;
    base_mutex(base_mutex &&) = delete;
    virtual ~base_mutex();

    base_mutex &operator=(const base_mutex &) = delete;
    base_mutex &operator=(const base_mutex &&) = delete;

    virtual void lock();
    virtual bool try_lock();
    virtual void unlock();
    virtual void unlock_ignore_owner();

    virtual bool timed_lock(uint64_t wait_in_us);
    virtual bool am_owner();

  protected:
    ipc::spinlock access_lock; ///< Simple lock protecting the rest of this mutex's fields.

    bool recursive; ///< Is this a recursive mutex?

    /// How many times has this mutex been locked. 0 and 1 are always valid, >1 is only valid for recursive mutexes.
    uint64_t lock_count{0};

    task_thread *owner_thread{nullptr}; ///< Which thread owns the mutex right now?

    /// Which processes are waiting to grab this mutex?
    klib_list<std::shared_ptr<task_thread>> waiting_threads_list;
  };

  class mutex : public base_mutex, public virtual IHandledObject
  {
  public:
    mutex() noexcept;
    mutex(bool recursive) noexcept;
    mutex(mutex &) = delete;
    mutex(mutex &&) = delete;
    virtual ~mutex();

    mutex &operator=(const mutex &) = delete;
    mutex &operator=(const mutex &&) = delete;
  };
};
