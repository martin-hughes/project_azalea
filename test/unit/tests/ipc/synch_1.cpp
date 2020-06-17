// Simple lock/unlock tests of spinlocks.

#include <iostream>
#include <thread>
#include "gtest/gtest.h"

#include "types/spinlock.h"
#include "test_core/test.h"

using namespace std;

void test_1_second_part();
void test_2_second_part();

namespace
{
  ipc::raw_spinlock main_lock;
  volatile bool lock_locked = false;

  volatile bool thread_1_locked = false;
  volatile bool thread_2_locked = false;
}

TEST(IPC, Spinlocks1)
{
  cout << "Synch test 1 - Spinlocks." << endl;
  cout << "This test takes 10 seconds to complete." << endl;

  ipc_raw_spinlock_init(main_lock);
  thread other_thread(test_1_second_part);
  while (!lock_locked)
  {
    // Spin! Wait for the lock to be locked, otherwise there's a chance that the next statement will lock the lock
    // immediately, thus invalidating the test.
  }
  ipc_raw_spinlock_lock(main_lock);
  ASSERT_FALSE(lock_locked);
  ipc_raw_spinlock_unlock(main_lock);

  ASSERT_TRUE(ipc_raw_spinlock_try_lock(main_lock));
  ASSERT_FALSE(ipc_raw_spinlock_try_lock(main_lock));
  ipc_raw_spinlock_unlock(main_lock);
  ipc_raw_spinlock_lock(main_lock);
  ASSERT_FALSE(ipc_raw_spinlock_try_lock(main_lock));
  ipc_raw_spinlock_unlock(main_lock);

  other_thread.join();
}

void test_1_second_part()
{
  ipc_raw_spinlock_lock(main_lock);
  lock_locked = true;
  this_thread::sleep_for(chrono::seconds(10));
  lock_locked = false;
  ipc_raw_spinlock_unlock(main_lock);
}

// This test aggressively locks and unlocks the lock to see if both threads ever think they're locked at the same time.
TEST(IPC, Spinlocks2)
{
  cout << "This test takes several seconds to complete." << endl;

  ipc_raw_spinlock_init(main_lock);
  thread other_thread(test_2_second_part);

  const uint32_t cycles = 100000;
  for (uint32_t i = 0; i < cycles; i++)
  {
    ipc_raw_spinlock_lock(main_lock);
    ASSERT_FALSE(thread_2_locked);
    thread_1_locked = true;
    test_spin_sleep(10000);
    thread_1_locked = false;
    ASSERT_FALSE(thread_2_locked);
    ipc_raw_spinlock_unlock(main_lock);
  }

  other_thread.join();
}

void test_2_second_part()
{
  const uint32_t cycles = 111111;
  for (uint32_t i = 0; i < cycles; i++)
  {
    ipc_raw_spinlock_lock(main_lock);
    ASSERT_FALSE(thread_1_locked);
    thread_2_locked = true;
    test_spin_sleep(9000);
    thread_2_locked = false;
    ASSERT_FALSE(thread_1_locked);
    ipc_raw_spinlock_unlock(main_lock);
  }
}
