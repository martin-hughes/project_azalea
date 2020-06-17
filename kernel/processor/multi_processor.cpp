/// @file
/// @brief Platform-agnostic processor control functions

// Known defects:
// - What happens if processors are not IDd sequentially? Will the proc_info_block array still work?

//#define ENABLE_TRACING

#include "processor.h"
#include "processor-int.h"

extern processor_info *proc_info_block;
/// Processor information storage for each processor, as an array indexed by processor ID.
processor_info *proc_info_block = nullptr;

extern uint32_t processor_count;
/// How many processors are known to the system?
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
      proc_start_interrupts();
      asm("hlt");
      break;

    case PROC_IPI_MSGS::SUSPEND:
      proc_stop_this_proc();
      break;

    case PROC_IPI_MSGS::TLB_SHOOTDOWN:
      mem_invalidate_tlb();
      break;

    case PROC_IPI_MSGS::RELOAD_IDT:
      proc_install_idt();
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
      proc_mp_signal_processor(i, PROC_IPI_MSGS::SUSPEND, false);
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
      proc_mp_signal_processor(i, PROC_IPI_MSGS::RESUME, true);
    }
  }

  KL_TRC_EXIT;
}

/// @brief Send an IPI message to all processors, including the one running this code.
///
/// @param msg The message to send to all processors.
///
/// @param exclude_self If set to true, the message is sent to all processors except this one. Note that this function
///                     may move between processors as part of the threading process. The processor excluded will be
///                     the one that this function was running on at the time the function starts.
///
/// @param wait_for_complete If true, wait for each processor to handle this message in sequence. Don't return until
///                          all processors have handled the message.
void proc_mp_signal_all_processors(PROC_IPI_MSGS msg, bool exclude_self, bool wait_for_complete)
{
  uint32_t this_proc = proc_mp_this_proc_id();

  KL_TRC_ENTRY;

  for (uint32_t i = 0; i < processor_count; i++)
  {
    if (!(exclude_self && (i == this_proc)))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Signal processor ", i, "\n");
      proc_mp_signal_processor(i, msg, wait_for_complete);
    }
  }

  KL_TRC_EXIT;
}
