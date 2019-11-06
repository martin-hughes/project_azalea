/// @file
/// @brief Declare a mutex class.

#pragma once

#include "klib/synch/kernel_locks.h"
#include "processor/processor.h"

#include <memory>

const uint64_t MUTEX_MAX_WAIT = 0xFFFFFFFFFFFFFFFF; ///< Constant defining an infinite wait to lock a mutex.

/// @brief A mutex structure used for basic locking.
///
/// There's no inherent reason this couldn't be the basis of a mutex for user space too, but it'd need wrapping in some
/// kind of handle. Users of mutexes shouldn't modify this structure, or they could cause problems with
/// synchronization.
struct klib_mutex
{
  bool mutex_locked{false}; ///<Is the mutex locked by any process?
  task_thread *owner_thread{nullptr}; ///< Which process has locked the thread?

  /// Which processes are waiting to grab this mutex?
  ///
  klib_list<std::shared_ptr<task_thread>> waiting_threads_list{nullptr, nullptr};

  /// This lock is used to synchronize access to the fields in this structure.
  ///
  kernel_spinlock access_lock{0};
};

void klib_synch_mutex_init(klib_mutex &mutex);
SYNC_ACQ_RESULT klib_synch_mutex_acquire(klib_mutex &mutex, uint64_t max_wait);
void klib_synch_mutex_release(klib_mutex &mutex, const bool disregard_owner);
