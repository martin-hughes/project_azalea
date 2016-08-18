/// @file
/// @brief Supports multi-processor operations.
///
/// Supports multi-processor operations.
/// Allows:
/// - Processors to be enumerated and identified
/// - Processors to be started and stopped
/// - Signals to be sent between processors.
///
/// Functions in this file that do not contain _x64 in their name would be generic to all platforms, but the exact
/// implementation is platform specific.

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/x64/processor-x64-int.h"
#include "processor/x64/pic/pic.h"

// Continue initialisation such that the other processors can be started, but leave them idle for now.
void proc_mp_init()
{
  KL_TRC_ENTRY;

  // Prepare the IO-APICs, these will be useful for distributing interrupts between processors later on.
  proc_conf_local_int_controller();
  proc_configure_global_int_ctrlrs();

  // Now all interrupt controllers needed for the BSP are good to go. Enable interrupts.
  asm_proc_start_interrupts();

  KL_TRC_EXIT;
}

/// @brief Return the number of processors in the system.
///
/// For the time being, this is limited to 1.
///
/// @return The number of processors in the system. Processors can then by identified by an integer in the range 0 ->
///         (return value - 1)
unsigned int proc_mp_proc_count()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT

  return 1;
}

/// @brief Return the ID number of this processor
///
/// Until multi-processing is supported, this will always return 0.
///
/// @return The integer ID number of the processor this function executes on.
unsigned int proc_mp_this_proc_id()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;

  return 0;
}
