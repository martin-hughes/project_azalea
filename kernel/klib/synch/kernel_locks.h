#ifndef _KLIB_SYNCH_K_LOCKS
#define _KLIB_SYNCH_K_LOCKS

#include <stdint.h>
#include <atomic>

typedef std::atomic<uint64_t> kernel_spinlock;

void klib_synch_spinlock_init(kernel_spinlock &lock);
extern "C" void klib_synch_spinlock_lock(kernel_spinlock &lock);
bool klib_synch_spinlock_try_lock(kernel_spinlock &lock);
extern "C" void klib_synch_spinlock_unlock(kernel_spinlock &lock);

enum SYNC_ACQ_RESULT
{
  SYNC_ACQ_ACQUIRED,
  SYNC_ACQ_TIMEOUT,
  SYNC_ACQ_ALREADY_OWNED,
};

#endif
