/// @file
/// @brief Implement semaphores within the Azalea kernel.

//#define ENABLE_TRACING

#include "tracing.h"
#include "k_assert.h"
#include "panic.h"
#include "types/semaphore.h"
#include "types/list.h"
#include "processor.h"
#include "timing.h"


ipc::semaphore::semaphore(uint64_t max_users, uint64_t start_users) : cur_user_count{start_users}, max_users{max_users}
{
  KL_TRC_ENTRY;

  klib_list_initialize(&waiting_threads_list);

  KL_TRC_EXIT;
}

ipc::semaphore::~semaphore()
{
  KL_TRC_ENTRY;

  ASSERT(klib_list_is_empty(&waiting_threads_list));

  KL_TRC_EXIT;
}

void ipc::semaphore::wait()
{
  timed_wait(ipc::MAX_WAIT);
}

bool ipc::semaphore::timed_wait(uint64_t wait_in_us)
{
  bool result;

  KL_TRC_ENTRY;

  access_lock.lock();

  if (cur_user_count < max_users)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Immediately acquired\n");
    cur_user_count++;
    result = true;
  }
  else if (wait_in_us == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No spare slots and immediate fallback\n");
    result = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore full, timed or indefinite wait.\n");

    // Wait for the semaphore to become free. Add this thread to the list of waiting threads, then suspend this thread.
    task_thread *this_thread = task_get_cur_thread();
    klib_list_item<std::shared_ptr<task_thread>> *item = new klib_list_item<std::shared_ptr<task_thread>>;

    ASSERT(cur_user_count == max_users);

    klib_list_item_initialize(item);
    item->item = this_thread->synch_list_item->item;

    klib_list_add_tail(&waiting_threads_list, item);

    // To avoid marking this thread as not being scheduled before freeing the lock - which would deadlock anyone else
    // trying to use this semaphore - stop scheduling for the time being.
    task_continue_this_thread();
    this_thread->stop_thread();

    // If there is a period to wait then specify it to the scheduler now. The scheduler won't react until after
    // scheduling is resumed.
    if (wait_in_us != ipc::MAX_WAIT)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set thread wakeup time\n");
      this_thread->wake_thread_after = time_get_system_timer_count(true) + (1000 * wait_in_us);
    }

    // Freeing the lock means that we could immediately become the owner thread. That's OK, we'll check once we come
    // back to this code after yielding.
    access_lock.unlock();

    // Don't yield without resuming normal scheduling, otherwise we'll come straight back here without acquiring the
    // semaphore. Once task_yield is called, the scheduler won't resume this process because it has been removed from
    // the running list by task_stop_thread.
    task_resume_scheduling();
    task_yield();

    // We've been scheduled again! Perhaps we've been signalled past the semaphore?
    access_lock.lock();

    // If this thread still appears in the waiting threads list then we've simply timed out.
    if (!klib_list_item_is_in_any_list(this_thread->synch_list_item))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully acquired\n");
      result = true;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Simply timed out.\n");
      result = false;

      item = waiting_threads_list.head;
      while (item)
      {
        if (item->item == this_thread->synch_list_item->item)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found this thread in the waiting list\n");
          klib_list_remove(item);
          delete item;
          item = nullptr;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Move to next item\n");
          item = item->next;
        }
      }
    }
  }

  access_lock.unlock();

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void ipc::semaphore::clear()
{
  KL_TRC_ENTRY;

  klib_list_item<std::shared_ptr<task_thread>> *next_owner;

  access_lock.lock();

  next_owner = waiting_threads_list.head;
  if (next_owner == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No next user for the semaphore, release\n");
    ASSERT(cur_user_count > 0);
    cur_user_count--;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Getting next user from the head of list\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Next user is", next_owner->item.get(), "\n");
    ASSERT(cur_user_count == max_users);
    klib_list_remove(next_owner);
    next_owner->item->start_thread();
    delete next_owner;
    next_owner = nullptr;
  }

  access_lock.unlock();

  KL_TRC_EXIT;

}
