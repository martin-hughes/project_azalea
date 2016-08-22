// Klib-synch test script 2.
//
// Simple lock/unlock tests of mutexes.

#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "klib/synch/kernel_mutexes.h"
#include "synch_test_list.h"
#include "test/test_core/test.h"
#include "processor/processor.h"

using namespace std;

volatile bool test2_lock_locked = false;

void* test2_second_part(void *);

void synch_test_2()
{
  pthread_t other_thread;

  cout << "Synch test 2 - Mutexes." << endl;

  klib_mutex main_mutex;

  cout << "Single thread acquire and release" << endl;
  klib_synch_mutex_init(main_mutex);
  klib_synch_mutex_acquire(main_mutex, MUTEX_MAX_WAIT);
  klib_synch_mutex_release(main_mutex);


}

void* test2_second_part(void *)
{

  return nullptr;
}
