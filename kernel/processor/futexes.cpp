/// @file
/// @brief Implement futexes in the Azalea kernel.
///
/// See the Linux futex and robust futex documentation for a fuller description of how futexes work.

#include "futexes.h"
#include "processor/processor.h"
#include "klib/klib.h"

#include <map>
#include <vector>

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
ERR_CODE futex_wait(uint64_t phys_addr, int32_t cur_value, int32_t req_value)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};

  KL_TRC_ENTRY;

  if (cur_value == req_value)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Need to wait\n");

    // This sequence of continuing execution even after calling stop_thread() is similar to that used for mutexes and
    // semaphores.
    klib_synch_spinlock_lock(map_ops_lock);
    task_continue_this_thread();

    if(!map_contains(*futex_map, phys_addr))
    {
      futex_map->insert({phys_addr, std::vector<task_thread *>()});
    }

    std::vector<task_thread *> &v = (*futex_map)[phys_addr];
    v.push_back(task_get_cur_thread());

    task_get_cur_thread()->stop_thread();
    klib_synch_spinlock_unlock(map_ops_lock);
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
ERR_CODE futex_wake(uint64_t phys_addr)
{
  ERR_CODE result{ERR_CODE::NO_ERROR};

  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(map_ops_lock);

  if (map_contains(*futex_map, phys_addr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found physical address, wake any sleepers\n");

    for(task_thread *sleeper : (*futex_map)[phys_addr])
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wake thread with address: ", sleeper, "\n");
      sleeper->start_thread();
    }
    futex_map->erase(phys_addr);
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
