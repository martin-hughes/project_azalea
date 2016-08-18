#include "kernel_locks.h"
#include "klib/klib.h"
#include "processor/processor.h"

// TODO: Add some more data to the spinlocks, e.g. an initialised flag?

void klib_synch_spinlock_init(kernel_spinlock &lock)
{
  KL_TRC_ENTRY;

  lock = 0;

  KL_TRC_EXIT;
}

void klib_synch_spinlock_lock(kernel_spinlock &lock)
{
  KL_TRC_ENTRY;

  asm_klib_synch_spinlock_lock(&lock);

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

  res = asm_klib_synch_spinlock_try_lock(&lock) == 1 ? true : false;

  KL_TRC_EXIT

  return res;
}
