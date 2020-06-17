/// @file
/// @brief Declares a semaphore for use in the kernel.

#pragma once

#include "ipc_core.h"
#include "handled_obj.h"

#include "types/thread.h"
#include "types/list.h"

namespace ipc
{
  class semaphore final : public virtual IHandledObject
  {
  public:
    semaphore(uint64_t max_users, uint64_t start_users = 0);
    semaphore(const semaphore &) = delete;
    semaphore(const semaphore &&) = delete;
    ~semaphore();

    semaphore &operator=(const semaphore &) = delete;
    semaphore &operator=(const semaphore &&) = delete;

    void wait();
    bool timed_wait(uint64_t wait_in_us);
    void clear();

    /// @brief How many threads is the semaphore being held by?
    ///
    uint64_t cur_user_count;

    /// @brief How many threads can hold the semaphore at once?
    ///
    uint64_t max_users;

    /// @brief Which processes are waiting to grab this semaphore?
    ///
    klib_list<std::shared_ptr<task_thread>> waiting_threads_list;

    /// @brief This lock is used to synchronize access to the fields in this structure.
    ///
    ipc::spinlock access_lock;
  };
};
