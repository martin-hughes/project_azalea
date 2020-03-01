// Tests of kernel_spinlock_obj and std::lock_guard

#include "gtest/gtest.h"
#include "klib/klib.h"

#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

void test_3_second_part();

namespace
{
  kernel_spinlock_obj main_lock;
  volatile bool lock_locked = false;

  volatile bool thread_1_locked = false;
  volatile bool thread_2_locked = false;
}

TEST(KlibSpinlockTest, Spinlocks3Wrapper)
{
  cout << "Synch test 3 - Spinlock wrappers." << endl;
  cout << "This test takes 10 seconds to complete." << endl;

  thread other_thread(test_3_second_part);
  while (!lock_locked)
  {
    // Spin! Wait for the lock to be locked, otherwise there's a chance that the next statement will lock the lock
    // immediately, thus invalidating the test.
  }

  {
    std::scoped_lock<kernel_spinlock_obj> guard(main_lock);
    ASSERT_FALSE(lock_locked);
  }

  other_thread.join();
}

void test_3_second_part()
{
  std::scoped_lock<kernel_spinlock_obj> guard(main_lock);
  lock_locked = true;
  this_thread::sleep_for(chrono::seconds(10));
  lock_locked = false;
}