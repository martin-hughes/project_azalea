// Supports multi-processor operation.
// Allows:
// - Processors to be started and stopped
// - Signals to be sent between processors.

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/x64/processor-x64-int.h"
#include "processor/x64/pic/pic.h"

// Continue initialization such that the other processors can be started, but leave them idle for now.
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
