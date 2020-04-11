/// @file
/// @brief Implement futexes in the Azalea kernel.
///
/// See the Linux futex and robust futex documentation for a fuller description of how futexes work.

//#define ENABLE_TRACING

#include "futexes.h"
#include "processor/processor.h"
#include "klib/klib.h"

#include <map>
#include <vector>
#include <algorithm>

namespace
{
  kernel_spinlock map_ops_lock{0}; ///< Lock protecting the futex map, below.
  std::map<uint64_t, std::vector<task_thread *>> *futex_map{nullptr}; ///< Map of all futexes known in the system.
}

/// @brief If needed, initialize the futex system.
///
void futex_maybe_init()
{
  KL_TRC_ENTRY;

  if(!futex_map)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Maybe construct system futex map\n");
    klib_synch_spinlock_lock(map_ops_lock);
    if (!futex_map)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Really create system futex map\n");
      futex_map = new std::map<uint64_t, std::vector<task_thread *>>();
    }
    klib_synch_spinlock_unlock(map_ops_lock);
  }

  KL_TRC_EXIT;
}

/// @brief Wait for the requested futex.
///
/// @param phys_addr Physical address of the futex being waited on
///
/// @param cur_value Current value of the futex as retrieved by atomic operation
///
/// @param req_value The value of the desired futex state given in the system call.
///
/// @return A suitable error code.
ERR_CODE futex_wait(volatile int32_t *futex, int32_t req_value)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  uint64_t futex_addr_l{reinterpret_cast<uint64_t>(futex)};

  KL_TRC_ENTRY;

  if (*futex == req_value)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Need to wait\n");

    // This sequence of continuing execution even after calling stop_thread() is similar to that used for mutexes and
    // semaphores.
    klib_synch_spinlock_lock(map_ops_lock);
    task_continue_this_thread();

    if(!map_contains(*futex_map, futex_addr_l))
    {
      futex_map->insert({futex_addr_l, std::vector<task_thread *>()});
    }

    std::vector<task_thread *> &v = (*futex_map)[futex_addr_l];
    v.push_back(task_get_cur_thread());

    task_get_cur_thread()->stop_thread();
    klib_synch_spinlock_unlock(map_ops_lock);

    if (*futex != req_value)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wake this thread, just in case\n");
      klib_synch_spinlock_lock(map_ops_lock);
      std::vector<task_thread *>::iterator it = std::find(v.begin(), v.end(), task_get_cur_thread());

      if (it != v.end())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Thread hasn't been woken externally\n");
        v.erase(it);

        if (v.empty())
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No more waits on this mutex\n");
          futex_map->erase(futex_addr_l);
        }
      }

      klib_synch_spinlock_unlock(map_ops_lock);
      task_get_cur_thread()->start_thread();
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
/// @param phys_addr Physical address of the futex to wake.
///
/// @return A suitable error code.
ERR_CODE futex_wake(volatile int32_t *futex)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};
  uint64_t futex_addr_l{reinterpret_cast<uint64_t>(futex)};

  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(map_ops_lock);

  if (map_contains(*futex_map, futex_addr_l))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found physical address, wake any sleepers\n");

    for(task_thread *sleeper : (*futex_map)[futex_addr_l])
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wake thread with address: ", sleeper, "\n");
      sleeper->start_thread();
    }
    futex_map->erase(futex_addr_l);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Didn't find physical address\n");
    result = ERR_CODE::NOT_FOUND;
  }

  klib_synch_spinlock_unlock(map_ops_lock);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
