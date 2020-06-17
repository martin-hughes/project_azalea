/// @file
/// @brief Implement futexes in the Azalea kernel.
///
/// See the Linux futex and robust futex documentation for a fuller description of how futexes work.

//#define ENABLE_TRACING

#include "types/futex.h"
#include "processor.h"
#include "map_helpers.h"

#include <map>
#include <vector>
#include <algorithm>

/// @brief Wait for the requested futex.
///
/// @param futex The futex being waited on
///
/// @param req_value The value of the desired futex state given in the system call.
///
/// @return A suitable error code.
ERR_CODE futex_wait(volatile int32_t *futex, int32_t req_value)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  uint64_t futex_addr_l{reinterpret_cast<uint64_t>(futex)};
  task_thread *cur_thread = task_get_cur_thread();
  ASSERT(cur_thread);
  std::shared_ptr<task_process> cur_process{cur_thread->parent_process};
  ASSERT(cur_process);

  KL_TRC_ENTRY;

  if (*futex == req_value)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Need to wait\n");

    // This sequence of continuing execution even after calling stop_thread() is similar to that used for mutexes and
    // semaphores.
    ipc_raw_spinlock_lock(cur_process->map_ops_lock);
    task_continue_this_thread();

    if(!map_contains(cur_process->futex_map, futex_addr_l))
    {
      cur_process->futex_map.insert({futex_addr_l, std::vector<task_thread *>()});
    }

    std::vector<task_thread *> &v = cur_process->futex_map[futex_addr_l];
    v.push_back(cur_thread);

    cur_thread->stop_thread();
    ipc_raw_spinlock_unlock(cur_process->map_ops_lock);

    if (*futex != req_value)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wake this thread, just in case\n");
      ipc_raw_spinlock_lock(cur_process->map_ops_lock);
      std::vector<task_thread *>::iterator it = std::find(v.begin(), v.end(), cur_thread);

      if (it != v.end())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Thread hasn't been woken externally\n");
        v.erase(it);

        if (v.empty())
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No more waits on this mutex\n");
          cur_process->futex_map.erase(futex_addr_l);
        }
      }

      ipc_raw_spinlock_unlock(cur_process->map_ops_lock);
      cur_thread->start_thread();
    }

    task_resume_scheduling();
    task_yield();
  }
  // Else no need to wait

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Wake the requested futex
///
/// @param futex The futex to wake.
///
/// @return A suitable error code.
ERR_CODE futex_wake(volatile int32_t *futex)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  uint64_t futex_addr_l{reinterpret_cast<uint64_t>(futex)};
  task_thread *cur_thread = task_get_cur_thread();
  ASSERT(cur_thread);
  std::shared_ptr<task_process> cur_process{cur_thread->parent_process};
  ASSERT(cur_process);

  KL_TRC_ENTRY;

  ipc_raw_spinlock_lock(cur_process->map_ops_lock);

  if (map_contains(cur_process->futex_map, futex_addr_l))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found physical address, wake any sleepers\n");

    for(task_thread *sleeper : cur_process->futex_map[futex_addr_l])
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wake thread with address: ", sleeper, "\n");
      sleeper->start_thread();
    }
    cur_process->futex_map.erase(futex_addr_l);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Didn't find physical address\n");
    result = ERR_CODE::NOT_FOUND;
  }

  ipc_raw_spinlock_unlock(cur_process->map_ops_lock);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
