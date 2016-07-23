// Klib-synch test script 1.
//
// Simple lock/unlock tests of spinlocks.

#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "klib/synch/kernel_locks.h"
#include "synch_test_list.h"
#include "test/test_core/test.h"

using namespace std;

kernel_spinlock main_lock;
volatile bool lock_locked = false;

void* second_part(void *);

void synch_test_1()
{
  pthread_t other_thread;

  cout << "Synch test 1 - Spinlocks." << endl;
  cout << "This test takes 10 seconds to complete." << endl;

  klib_synch_spinlock_init(main_lock);
  pthread_create(&other_thread, NULL, &second_part, NULL);
  while (!lock_locked)
  {
    // Spin! Wait for the lock to be locked, otherwise there's a chance that the next statement will lock the lock
    // immediately, this invalidating the test.
  }
  klib_synch_spinlock_lock(main_lock);
  ASSERT(!lock_locked);
  klib_synch_spinlock_unlock(main_lock);
}

void* second_part(void *)
{
  klib_synch_spinlock_lock(main_lock);
  lock_locked = true;
  sleep(10);
  lock_locked = false;
  klib_synch_spinlock_unlock(main_lock);

  return NULL;
}
