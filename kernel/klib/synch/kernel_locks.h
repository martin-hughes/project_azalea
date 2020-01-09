/// @file
/// @brief Declares simple spinlock functionality.

#pragma once

#include <stdint.h>
#include <atomic>

typedef std::atomic<uint64_t> kernel_spinlock; ///< Kernel spinlock type.

void klib_synch_spinlock_init(kernel_spinlock &lock);
extern "C" void klib_synch_spinlock_lock(kernel_spinlock &lock);
bool klib_synch_spinlock_try_lock(kernel_spinlock &lock);
extern "C" void klib_synch_spinlock_unlock(kernel_spinlock &lock);

/// @brief Standard results for attempting to lock any synch object.
///
enum SYNC_ACQ_RESULT
{
  SYNC_ACQ_ACQUIRED, ///< The object was acquired.
  SYNC_ACQ_TIMEOUT, ///< The object could not be acquired within the required time.
  SYNC_ACQ_ALREADY_OWNED, ///< This thread already owns this object.
};

/// @brief Wrapper class around kernel_spinlock
///
/// This will allow the use of spinlocks with std::lock_guard and make an RAII style of lock usage a bit easier.
class kernel_spinlock_obj
{
public:
  kernel_spinlock_obj();
  kernel_spinlock_obj(kernel_spinlock_obj &) = delete;
  kernel_spinlock_obj(kernel_spinlock_obj &&) = delete;
  ~kernel_spinlock_obj();

  kernel_spinlock underlying_lock;

  // To satisfy BasicLockable.
  void lock();
  void unlock();
};
