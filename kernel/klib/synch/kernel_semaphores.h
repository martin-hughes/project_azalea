#ifndef KLIB_SEMAPHORE
#define KLIB_SEMAPHORE

#include <stdint.h>

#include "klib/synch/kernel_locks.h"
#include "processor/processor.h"

const uint64_t SEMAPHORE_MAX_WAIT = 0xFFFFFFFFFFFFFFFF;

// Defines a semaphore structure. There's no inherent reason this couldn't be the basis of a semaphore for user space
// too, but it'd need wrapping in some kind of handle. Users of semaphores shouldn't modify this structure, or they
// could cause problems with synchronization.
struct klib_semaphore
{
  // How many threads is the semaphore being held by?
  uint64_t cur_user_count;

  // How many threads can hold the semaphore at once?
  uint64_t max_users;

  // Which processes are waiting to grab this mutex?
  klib_list<task_thread *> waiting_threads_list;

  // This lock is used to synchronize access to the fields in this structure.
  kernel_spinlock access_lock;
};

void klib_synch_semaphore_init(klib_semaphore &semaphore, uint64_t max_users, uint64_t start_users = 0);
SYNC_ACQ_RESULT klib_synch_semaphore_wait(klib_semaphore &semaphore, uint64_t max_wait);
void klib_synch_semaphore_clear(klib_semaphore &semaphore);

#endif
