#include "kernel_locks.h"
#include "klib/klib.h"
#include "processor/processor.h"
#include <atomic>

void klib_synch_spinlock_init(kernel_spinlock &lock)
{
  KL_TRC_ENTRY;

  lock = 0;

  KL_TRC_EXIT;
}

void klib_synch_spinlock_lock(kernel_spinlock &lock)
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

void klib_synch_spinlock_unlock(kernel_spinlock &lock)
{
  KL_TRC_ENTRY;

  lock = 0;

  KL_TRC_EXIT;
}

bool klib_synch_spinlock_try_lock(kernel_spinlock &lock)
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
