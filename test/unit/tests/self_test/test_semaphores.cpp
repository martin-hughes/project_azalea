/// @file
/// @brief Tests the implementation of Semaphores used in the test scripts.
///
/// This is only really a sanity check, if any problems appear in the tests then it might be worth doing more detailed
/// tests

#include <iostream>
#include <thread>
#include "gtest/gtest.h"

#include "types/semaphore.h"
#include "test_core/test.h"

using namespace std;

namespace
{
  void semaphores_1_second_part();
  bool second_thread_flag{false};
  std::unique_ptr<ipc::semaphore> sem;
}

TEST(SelfTest, Semaphores1)
{
  cout << "This test takes 10 seconds to complete. If it takes much longer, it has hung." << endl;

  sem = std::make_unique<ipc::semaphore>(2, 0);

  second_thread_flag = false;

  // These two waits should be OK.
  sem->wait(); // 1 user
  sem->wait(); // 2 users

  sem->clear(); // Back to 1 user

  sem->wait(); // 2 users

  thread other_thread(semaphores_1_second_part);
  sem->wait(); // Now we should wait.

  ASSERT(second_thread_flag); // This almost certainly relies on not using optimizations.

  other_thread.join();

  sem.reset();
}

namespace
{
  void semaphores_1_second_part()
  {
    this_thread::sleep_for(chrono::seconds(10));
    sem->clear();

    second_thread_flag = true;
  }
}
