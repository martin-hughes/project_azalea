/// @file
/// @brief Kernel Handle Manager
///
/// This file allocates handles upon requests, and keeps track of their allocation and deallocation. It doesn't care
/// what the handles are used for, nor does it keep track of the relationship between handles and objects. That is the
/// work of the Object Manager
///
/// At the moment, this file doesn't even do a very good job of keeping track of handles. It simply allocates in an
/// upwards direction until it runs out, then crashes.

// Known defects:
// - hm_next_handle is obviously pretty crap anyway, but it could be replaced by an atomic...?

#include <stdint.h>

#include "tracing.h"
#include "panic.h"
#include "k_assert.h"

#include "handles.h"
#include "types/spinlock.h"

static GEN_HANDLE hm_next_handle = 1; ///< The number of the next handle to allocate (see note at top of handles.cpp)
static ipc::raw_spinlock hm_lock; ///< Lock protecting hm_next_handle.

/// @brief Initialise the Handle Manager component
void hm_gen_init()
{
  KL_TRC_ENTRY;

  ipc_raw_spinlock_init(hm_lock);

  KL_TRC_EXIT;
}

/// @brief Allocate a new handle
///
/// @return The allocated handle.
GEN_HANDLE hm_get_handle()
{
  KL_TRC_ENTRY;

  GEN_HANDLE return_handle;

  ipc_raw_spinlock_lock(hm_lock);
  return_handle = hm_next_handle++;

  if (hm_next_handle == ~((uint64_t)0))
  {
    panic("Out of handles!");
  }

  ipc_raw_spinlock_unlock(hm_lock);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Returning handle: ", return_handle, "\n");

  KL_TRC_EXIT;

  return return_handle;
}

/// @brief Release a handle
///
/// At the moment, the handle manager makes no attempt to track this!
///
/// @param handle The handle to release
void hm_release_handle(GEN_HANDLE handle)
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}
