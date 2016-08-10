// Exception handlers for the kernel.
// Page faults are handled in interrupts-x64.cpp

//#define ENABLE_TRACING

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

void proc_nmi_int_handler()
{
  panic("NMI received");
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

void proc_double_fault_abort_handler(unsigned long err_code)
{
  panic("Double-fault");
}

void proc_invalid_tss_fault_handler(unsigned long err_code)
{
  panic("Invalid TSS");
}

void proc_seg_not_present_fault_handler(unsigned long err_code)
{
  panic("Segment not present");
}

void proc_ss_fault_handler(unsigned long err_code)
{
  panic("Stack selector fault");
}

void proc_gen_prot_fault_handler(unsigned long err_code)
{
  KL_TRC_DATA("GPF. Error code", err_code);
  panic("General protection fault");
}

void proc_fp_except_fault_handler()
{
  panic("Floating point exception fault");
}

void proc_align_check_fault_handler(unsigned long err_code)
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

void proc_security_fault_handler(unsigned long err_code)
{
  panic("Security fault");
}
