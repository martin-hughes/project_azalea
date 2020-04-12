#include "gtest/gtest.h"

#include <azalea/azalea.h>

#include <iostream>
#include <pthread.h>
#include <time.h>

using namespace std;

namespace
{
  void *other_thread(void *_);
  bool other_thread_fault{false};

  pthread_mutex_t t_lock;
  volatile int f{0};
}

TEST(PThread, BasicMutexWait)
{
  pthread_t thread_b;
  timespec a;
  timespec b;

  ASSERT_EQ(pthread_mutex_init(&t_lock, nullptr), 0);
  ASSERT_EQ(pthread_mutex_lock(&t_lock), 0);

  cout << "This test takes approximately 10 seconds" << endl;

  pthread_create(&thread_b, nullptr, other_thread, nullptr);

  a.tv_sec = 5;
  nanosleep(&a, &b);
  ASSERT_EQ(f, 0);

  ASSERT_EQ(pthread_mutex_unlock(&t_lock), 0);

  nanosleep(&a, &b);
  ASSERT_EQ(f, 1);

  pthread_join(thread_b, nullptr);

  ASSERT_FALSE(other_thread_fault);
}

namespace
{
  void *other_thread(void *_)
  {
    if (pthread_mutex_lock(&t_lock) != 0)
    {
      other_thread_fault = true;
    }
    if (pthread_mutex_unlock(&t_lock) != 0)
    {
      other_thread_fault = true;
    }
    f = 1;

    return nullptr;
  }
}
