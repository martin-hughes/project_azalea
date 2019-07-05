/// @file
/// @brief Platform-agnostic processor control functions

// Known defects:
// - What happens if processors are not IDd sequentially? Will the proc_info_block array still work?

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor.h"
#include "processor-int.h"
#include "x64/processor-x64.h"
#include "x64/processor-x64-int.h"
#include "x64/pic/apic.h"

/// Processor information storage for each processor, as an array indexed by processor ID.
extern processor_info *proc_info_block;
processor_info *proc_info_block = nullptr;

/// How many processors are known to the system?
extern uint32_t processor_count;
uint32_t processor_count = 0;

/// @brief Return the number of processors in the system.
///
/// @return The number of processors in the system. Processors can then by identified by an integer in the range 0 ->
///         (return value - 1)
uint32_t proc_mp_proc_count()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;

  return processor_count;
}

/// @brief Send a IPI signal to another processor
///
/// Inter-processor interrupts are used to signal control messages between processors. Control messages are defined in
/// PROC_IPI_MSGS.
///
/// @param proc_id The processor ID (not APIC ID) to signal.
///
/// @param msg The message to be sent.
void proc_mp_signal_processor(uint32_t proc_id, PROC_IPI_MSGS msg)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Message to send", static_cast<uint64_t>(msg), "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Processor to signal", proc_id, "\n");

  ASSERT(proc_id < processor_count);

  proc_mp_x64_signal_proc(proc_id, msg);

  KL_TRC_EXIT;
}

/// @brief Handle an signal sent from another processor to this one
///
/// @param msg The message received by this processor
void proc_mp_receive_signal(PROC_IPI_MSGS msg)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Received message", static_cast<uint64_t>(msg), "\n");

  switch (msg)
  {
    case PROC_IPI_MSGS::RESUME:
      asm_proc_start_interrupts();
      asm("hlt");
      break;

    case PROC_IPI_MSGS::SUSPEND:
      proc_stop_this_proc();
      break;

    case PROC_IPI_MSGS::TLB_SHOOTDOWN:
      INCOMPLETE_CODE(TLB SHOOTDOWN MSG);
      break;

    case PROC_IPI_MSGS::RELOAD_IDT:
      asm_proc_install_idt();
      break;

    default:
      panic("Unrecognised IP signal received.");
  }

  KL_TRC_EXIT;
}

/// @brief Stop all processors other than this one, with interrupts disabled on them.
void proc_stop_other_procs()
{
  KL_TRC_ENTRY;

  uint32_t this_proc_id = proc_mp_this_proc_id();
  KL_TRC_TRACE(TRC_LVL::EXTRA, "This processor ID", this_proc_id, "\n");

  for (uint32_t i = 0; i < processor_count; i++)
  {
    if ((this_proc_id != i) && (proc_info_block[i].processor_running))
    {
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Signalling processor", i, "\n");
      proc_mp_signal_processor(i, PROC_IPI_MSGS::SUSPEND);
    }
  }

  KL_TRC_EXIT;
}

/// @brief Stop all processors, with interrupts disabled.
///
/// This effectively crashes the system, no processor will receive an interrupt to continue unless external hardware
/// triggers an NMI - in which case, the behaviour is undefined.
void proc_stop_all_procs()
{
  KL_TRC_ENTRY;

  proc_stop_other_procs();
  proc_stop_this_proc();

  KL_TRC_EXIT;
}

/// @brief Start all Application Processors (APs)
///
/// Trigger all processors other than the BSP to begin executing.
///
/// They have been left halted with interrupts disabled by the bootloader (Pure64), so they are signalled by NMI to
/// come up, since the NMI isn't blocked.
void proc_mp_start_aps()
{
  KL_TRC_ENTRY;

  if (processor_count > 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Starting other processors\n");
    for (uint32_t i = 1; i < processor_count; i++)
    {
      proc_mp_signal_processor(i, PROC_IPI_MSGS::RESUME);
    }
  }

  KL_TRC_EXIT;
}

/// @brief Send an IPI message to all processors, including the one running this code.
///
/// @param msg The message to send to all processors.
void proc_mp_signal_all_processors(PROC_IPI_MSGS msg)
{
  for (uint32_t i = 0; i < processor_count; i++)
  {
    proc_mp_signal_processor(i, msg);
  }
}
