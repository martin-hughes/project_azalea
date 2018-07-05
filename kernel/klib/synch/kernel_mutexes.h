#ifndef _KLIB_MUTEX
#define _KLIB_MUTEX

#include "klib/synch/kernel_locks.h"
#include "processor/processor.h"

const uint64_t MUTEX_MAX_WAIT = 0xFFFFFFFFFFFFFFFF;

// Defines a mutex structure. There's no inherent reason this couldn't be the basis of a mutex for user space too, but
// it'd need wrapping in some kind of handle. Users of mutexes shouldn't modify this structure, or they could cause
// problems with synchronization.
struct klib_mutex
{
  // Is the mutex locked by any process?
  bool mutex_locked;

  // Which process has locked the thread?
  task_thread *owner_thread;

  // Which processes are waiting to grab this mutex?
  klib_list<task_thread *> waiting_threads_list;

  // This lock is used to synchronize access to the fields in this structure.
  kernel_spinlock access_lock;
};

void klib_synch_mutex_init(klib_mutex &mutex);
SYNC_ACQ_RESULT klib_synch_mutex_acquire(klib_mutex &mutex, uint64_t max_wait);
void klib_synch_mutex_release(klib_mutex &mutex, const bool disregard_owner);

#endif
