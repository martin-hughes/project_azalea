// Exception handlers for the kernel.
// Page faults are handled in interrupts-x64.cpp

#define ENABLE_TRACING

#include "processor/x64/processor-x64-int.h"
#include "klib/klib.h"

void proc_div_by_zero_fault_handler()
{
  panic("Divide by zero fault");
}

void proc_debug_fault_handler()
{
  panic("Debug fault");
}

void proc_brkpt_trap_handler()
{
  panic("Breakpoint hit");
}

void proc_overflow_trap_handler()
{
  panic("Overflow");
}

void proc_bound_range_fault_handler()
{
  panic("Bound range fault");
}

void proc_invalid_opcode_fault_handler()
{
  panic("Invalid opcode");
}

void proc_device_not_avail_fault_handler()
{
  panic("Device not available");
}

void proc_double_fault_abort_handler(uint64_t err_code, uint64_t rip)
{
  panic("Double-fault");
}

void proc_invalid_tss_fault_handler(uint64_t err_code, uint64_t rip)
{
  panic("Invalid TSS");
}

void proc_seg_not_present_fault_handler(uint64_t err_code, uint64_t rip)
{
  panic("Segment not present");
}

void proc_ss_fault_handler(uint64_t err_code, uint64_t rip)
{
  panic("Stack selector fault");
}

void proc_gen_prot_fault_handler(uint64_t err_code, uint64_t rip)
{
  KL_TRC_TRACE(TRC_LVL::ERROR, "GPF. Error code: ", err_code, "\n");
  KL_TRC_TRACE(TRC_LVL::ERROR, "RIP: ", rip, "\n");
  /*if (rip != 0)
  {
    KL_TRC_TRACE(TRC_LVL::ERROR,
                 "Instruction bytes * 8: ",
                 reinterpret_cast<uint64_t>(*reinterpret_cast<uint64_t *>(rip)),
                 "\n");
  }*/
  panic("General protection fault");
}

void proc_fp_except_fault_handler()
{
  panic("Floating point exception fault");
}

void proc_align_check_fault_handler(uint64_t err_code, uint64_t rip)
{
  panic("Alignment check fault");
}

void proc_machine_check_abort_handler()
{
  panic("Machine check failed");
}

void proc_simd_fpe_fault_handler()
{
  panic("SIMD floating point fault");
}

void proc_virt_except_fault_handler()
{
  panic("Virtualization exception");
}

void proc_security_fault_handler(uint64_t err_code, uint64_t rip)
{
  panic("Security fault");
}

/// @brief Handles page faults
///
/// Proper docs to follow when the system makes actual use of page faults.
///
/// @param fault_code See the Intel manual for more
/// @param fault_addr See the Intel manual for more
/// @param fault_instruction See the Intel manual for more
void proc_page_fault_handler(uint64_t fault_code, uint64_t fault_addr, uint64_t fault_instruction)
{
  KL_TRC_ENTRY;
  static bool in_page_fault = false;

  if (!in_page_fault)
  {
    in_page_fault = true;
    KL_TRC_TRACE(TRC_LVL::EXTRA, "fault code: ", fault_code, "\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "CR2 (bad mem address): ", fault_addr, "\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Instruction pointer: ", fault_instruction, "\n");
    if ((fault_code & 0x10) == 0)
    {
      // The fault wasn't caused by an instruction fetch, ergo we should be able to read & print the instruction...
      KL_TRC_TRACE(TRC_LVL::EXTRA,
                  "Instruction bytes x8: ", *(reinterpret_cast<uint64_t *>(fault_instruction)), "\n");
    }
    in_page_fault = false;
  }
  KL_TRC_EXIT;
  panic("Page fault!");
}