#include "gtest/gtest.h"

#include <pthread.h>

namespace
{
  void *other_thread(void *_);

  int send_val{5};
  int retval;

  volatile int comm{0};
}

TEST(PThread, BasicCreateAndJoin)
{
  void *ret_ptr;
  pthread_t thread_b;
  pthread_create(&thread_b, nullptr, other_thread, &send_val);

  // Wait for thread startup. If the thread never starts, the code will never pass the next line.
  while (comm != 2) { }

  pthread_join(thread_b, &ret_ptr);

  // This confirms both the sent and return value.
  ASSERT_EQ(ret_ptr, &retval);
}

namespace
{
  void *other_thread(void *sent)
  {
    void *r = &retval;
    int sent_i = *(int *)(sent);

    // Should still be in the initial state.
    if (comm != 0)
    {
      r = nullptr;
    }

    comm = 2;

    // Check the sent value, and communicate the result via the return value.
    if (sent_i != send_val)
    {
      r = nullptr;
    }

    return r;
  }
}
