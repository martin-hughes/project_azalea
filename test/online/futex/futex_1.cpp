#include "gtest/gtest.h"

#include <azalea/azalea.h>

#include <iostream>
#include <pthread.h>
#include <time.h>

using namespace std;

namespace
{
  void *other_thread(void *_);

  volatile int32_t f = 1;
}

TEST(Futex, BasicWaitAndWake)
{
  pthread_t thread_b;
  timespec a;
  timespec b;

  cout << "This test takes approximately 10 seconds" << endl;

  pthread_create(&thread_b, nullptr, other_thread, nullptr);

  a.tv_sec = 5;
  ASSERT_EQ(f, 1);

  nanosleep(&a, &b);
  ASSERT_EQ(f, 1);
  az_futex_op(&f, FUTEX_OP::FUTEX_WAKE, 0, 0, nullptr, 0);

  nanosleep(&a, &b);
  ASSERT_EQ(f, 2);

  pthread_join(thread_b, nullptr);
}

namespace
{
  void *other_thread(void *_)
  {
    az_futex_op(&f, FUTEX_OP::FUTEX_WAIT, 1, 0, nullptr, 0);
    f = 2;

    return nullptr;
  }
}
