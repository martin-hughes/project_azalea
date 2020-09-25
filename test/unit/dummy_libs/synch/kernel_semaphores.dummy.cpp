// Semaphore dummy implementation for test scripts.
// Since semaphores are untested, this is empty. For now.

//#define ENABLE_TRACING

#include <mutex>
#include <condition_variable>

#include "tracing.h"
#include "map_helpers.h"
#include "types/semaphore.h"

struct semaphore_details
{
  std::mutex mut;
  std::condition_variable cv;
};

std::map<ipc::semaphore *, std::unique_ptr<semaphore_details>> semaphore_map;

ipc::semaphore::semaphore(uint64_t max_users, uint64_t start_users) : max_users{max_users}, cur_user_count{start_users}
{
  ipc::semaphore *t = this;
  ASSERT(!map_contains(semaphore_map, t));
  semaphore_map[this] = std::make_unique<semaphore_details>();
}

ipc::semaphore::~semaphore()
{
  ipc::semaphore *t = this;
  ASSERT(map_contains(semaphore_map, t));
  semaphore_map.erase(this);
}

void ipc::semaphore::wait()
{
  timed_wait(ipc::MAX_WAIT);
}

bool ipc::semaphore::timed_wait(uint64_t wait_in_us)
{
  bool result{true};

  ipc::semaphore *t = this;
  ASSERT(map_contains(semaphore_map, t));

  std::unique_ptr<semaphore_details> &d = semaphore_map[t];

  std::unique_lock<std::mutex> lk(d->mut);

  auto test_fn = [this]{ return ( this->cur_user_count < this->max_users ); };

  if (wait_in_us != ipc::MAX_WAIT)
  {
    using mu_s = std::chrono::microseconds;
    result = d->cv.wait_for(lk, mu_s(wait_in_us), test_fn);
  }
  else
  {
    d->cv.wait(lk, test_fn);
  }

  if (result)
  {
    this->cur_user_count++;
  }

  ASSERT(this->cur_user_count <= this->max_users);

  return result;
}

void ipc::semaphore::clear()
{
  ipc::semaphore *t = this;
  ASSERT(map_contains(semaphore_map, t));

  std::unique_ptr<semaphore_details> &d = semaphore_map[t];

  std::unique_lock<std::mutex> lk(d->mut);
  ASSERT(this->cur_user_count > 0);
  this->cur_user_count--;

  d->cv.notify_one();
}
