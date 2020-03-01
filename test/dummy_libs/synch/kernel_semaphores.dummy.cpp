// KLIB semaphore dummy implementation for test scripts.
// Since semaphores are untested, this is empty. For now.

//#define ENABLE_TRACING

#include "klib/klib.h"

void klib_synch_semaphore_init(klib_semaphore &semaphore, uint64_t max_users, uint64_t start_users)
{
  INCOMPLETE_CODE("klib_synch_semaphore_init");
}

SYNC_ACQ_RESULT klib_synch_semaphore_wait(klib_semaphore &semaphore, uint64_t max_wait)
{
  INCOMPLETE_CODE("klib_synch_semaphore_wait");
}

void klib_synch_semaphore_clear(klib_semaphore &semaphore)
{
  INCOMPLETE_CODE("klib_synch_semaphore_clear");
}

