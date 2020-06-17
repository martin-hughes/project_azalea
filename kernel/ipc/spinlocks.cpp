/// @file
/// @brief Implements a basic spinlock.

#include "types/spinlock.h"
#include "processor.h"
#include <atomic>

/// @brief Initialise a kernel spinlock object
///
/// @param[in] lock The spinlock to initialise.
int ipc_raw_spinlock_init(ipc::raw_spinlock &lock)
{
  lock = 0;
  return 0;
};

/// @brief Acquire and lock a spinlock
///
/// This function will not return until it has locked the spinlock.
///
/// @param[in] lock The spinlock object to lock.
void ipc_raw_spinlock_lock(ipc::raw_spinlock &lock)
{
  KL_TRC_ENTRY;

  uint64_t expected = 0;
  uint64_t desired = 1;
  do
  {
    expected = 0;
    std::atomic_compare_exchange_strong(&lock, &expected, desired);
  } while (expected == 1);

  KL_TRC_EXIT;
}

/// @brief Unlock a previously locked kernel spinlock.
///
/// No checking is performed to ensure the owner is the one doing the unlocking.
///
/// @param[in] lock The lock to unlock.
void ipc_raw_spinlock_unlock(ipc::raw_spinlock &lock)
{
  KL_TRC_ENTRY;

  lock = 0;

  KL_TRC_EXIT;
}

/// @brief Try and lock a spinlock, but return immediately.
///
/// @param[in] lock The lock to try and lock.
///
/// @return True if the spinlock was acquired in this thread. False if it already had another owner.
bool ipc_raw_spinlock_try_lock(ipc::raw_spinlock &lock)
{
  KL_TRC_ENTRY;

  bool res;
  uint64_t expected = 0;
  uint64_t desired = 1;
  std::atomic_compare_exchange_weak(&lock, &expected, desired);

  res = (expected == 0);

  KL_TRC_EXIT;

  return res;
}

/// @brief Default destructor.
///
ipc::spinlock::~spinlock()
{
  KL_TRC_ENTRY;

  if(underlying_lock != 0)
  {
    ipc_raw_spinlock_unlock(underlying_lock);
  }

  KL_TRC_EXIT;
}

/// @brief Lock this lock object.
///
void ipc::spinlock::lock()
{
  KL_TRC_ENTRY;

  ipc_raw_spinlock_lock(underlying_lock);

  KL_TRC_EXIT;
}

/// @brief Unlock this lock object.
///
void ipc::spinlock::unlock()
{
  KL_TRC_ENTRY;

  ipc_raw_spinlock_unlock(underlying_lock);

  KL_TRC_EXIT;
}
