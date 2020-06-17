/// @file
/// @brief x64-specific processor management code.

//#define ENABLE_TRACING

#include "processor.h"
#include "../../../processor/processor-int.h"

#include "processor-x64.h"
#include "processor-x64-int.h"
#include "pic/pic.h"
#include "mem-x64.h"

/// @brief x64-specific per-processor information.
///
/// The indicies of this array mirror proc_info_block.
processor_info_x64 *proc_info_x64_block = nullptr;

/// @brief Initialise the first processor.
///
/// Does as much initialisation of the BSP as possible. We leave some of the harder stuff, like configuring the APIC
/// until after the memory manager is running.
void proc_gen_init()
{
  // Don't do any tracing in this function, since the tracing functions may rely on floating point math, and that isn't
  // enabled yet.

  // Interrupts should have been left disabled by the bootloader, but since
  // we're about to fiddle with the GDT, IDT and such, it's probably best
  // to make sure.
  asm_proc_stop_interrupts();

  // Enable the floating point units as well as SSE.
  asm_proc_enable_fp_math();

  // Set the current task to 0, since tasking isn't started yet and we don't want to accidentally believe we're running
  // a thread that doesn't exist.
  proc_write_msr(PROC_X64_MSRS::IA32_KERNEL_GS_BASE, 0);

  // Fill in the GDT, and select an appropriate set of segments. The TSS descriptor and segment will
  // come later.
  asm_proc_load_gdt();

  // Establish the interrupt data table.
  proc_config_interrupt_table();

  // Fill in the IDT now, so we at least handle our own exceptions.
  proc_configure_idt();

  // Further processor setup, including configuring PICs/APICs, continues after the memory mamanger is up.
}

/// @brief Cause this processor to enter the halted state.
void proc_stop_this_proc()
{
  asm_proc_stop_this_proc();
}

/// @brief Stop interrupts on this processor.
///
/// This function should be called with care - make sure to call `proc_start_interrupts` ASAP afterwards.
void proc_stop_interrupts()
{
  asm_proc_stop_interrupts();
}

/// @brief Start interrupts on this processor.
///
/// Care should be exercised when using this function. Do not start interrupts when you were not responsible for them
/// being stopped in the first place.
void proc_start_interrupts()
{
  asm_proc_start_interrupts();
}

/// @brief Read from a processor MSR
///
/// @param msr The MSR to read from.
///
/// @return The value of the MSR, combined in to a single 64 bit form.
uint64_t proc_read_msr(PROC_X64_MSRS msr)
{
  KL_TRC_ENTRY;

  uint64_t retval;
  uint64_t msr_l = static_cast<uint64_t>(msr);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Reading MSR", msr_l, "\n");
  retval = asm_proc_read_msr(msr_l);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Returned value", retval, "\n");

  KL_TRC_EXIT;

  return retval;
}

/// @brief Write to a processor MSR
///
/// @param msr The MSR to write to
///
/// @param value The 64-bit value to write out.
void proc_write_msr(PROC_X64_MSRS msr, uint64_t value)
{
  KL_TRC_ENTRY;

  uint64_t msr_l = static_cast<uint64_t>(msr);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Writing MSR", msr_l, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Value", value, "\n");

  asm_proc_write_msr(msr_l, value);

  KL_TRC_EXIT;
}

/// @brief Generate the contents of the MSI address register for PCI MSIs
///
/// This value can then be used in the PCI MSI capabilities register. At present, no attempt is made to support any of
/// the redirection features mentioned in the Intel System Programming Guide.
///
/// @param kernel_proc_id The ID of the processor to send messages to, as identified by the kernel.
///
/// @return A suitable address, if one could be generated, or zero otherwise.
uint64_t proc_x64_generate_msi_address(uint32_t kernel_proc_id)
{
  uint32_t lapic_id;
  uint64_t result;

  KL_TRC_ENTRY;

  ASSERT(processor_count > 0);
  if (kernel_proc_id >= processor_count)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid processor ID\n");
    result = 0;
  }
  else
  {
    lapic_id = proc_info_x64_block[kernel_proc_id].lapic_id;
    lapic_id = lapic_id & 0xff;

    result = 0xFEE00000 | (lapic_id << 12);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void proc_set_tls_register(TLS_REGISTERS reg, uint64_t value)
{
  KL_TRC_ENTRY;

  switch(reg)
  {
  case TLS_REGISTERS::FS:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Setting FS base to ", value, "\n");
    proc_write_msr (PROC_X64_MSRS::IA32_FS_BASE, value);
    break;

  case TLS_REGISTERS::GS:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Writing GS base to ", value, "\n");
    proc_write_msr (PROC_X64_MSRS::IA32_GS_BASE, value);
    break;

  default:
    KL_TRC_TRACE(TRC_LVL::ERROR, "proc_set_tls_register: Unknown register\n");
  }

  KL_TRC_EXIT;
}

/// @brief Install the current IDT on this processor.
///
void proc_install_idt()
{
  KL_TRC_ENTRY;

  asm_proc_install_idt();

  KL_TRC_EXIT;
}
