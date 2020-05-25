/// @file
/// @brief x64-specific processor management code.

//#define ENABLE_TRACING

#include "processor/processor.h"
#include "processor/processor-int.h"
#include "klib/klib.h"

#include "processor/x64/processor-x64.h"
#include "processor/x64/processor-x64-int.h"
#include "processor/x64/pic/pic.h"
#include "mem/x64/mem-x64-int.h"

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

/// @brief Read from a processor I/O port.
///
/// @param port_id The port to read from
///
/// @param width The number of bits to read. Must be one of 8, 16, or 32. If this does not correspond to the actual
///              width of the port being read, the processor may cause a GPF.
///
/// @return The value read from the port, zero-expanded to 64 bits.
uint64_t proc_read_port(const uint64_t port_id, const uint8_t width)
{
  KL_TRC_ENTRY;

  uint64_t retval;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Port", port_id, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Width", width, "\n");

  ASSERT((width == 8) || (width == 16) || (width == 32));

  retval = asm_proc_read_port(port_id, width);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Returned value", retval, "\n");
  KL_TRC_EXIT;

  return retval;
}

/// @brief Write to a processor I/O port
///
/// @param port_id The port to write to
///
/// @param value The value to write out
///
/// @param width The width of the port, in bits. Must be one of 8, 16 or 32 and must correspond to the I/O port's
///              actual width.
void proc_write_port(const uint64_t port_id, const uint64_t value, const uint8_t width)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Port", port_id, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Value", value, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Width", width, "\n");

  ASSERT((width == 8) || (width == 16) || (width == 32));

  asm_proc_write_port(port_id, value, width);

  KL_TRC_EXIT;
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
    lapic_id = proc_info_block[kernel_proc_id].platform_data.lapic_id;
    lapic_id = lapic_id & 0xff;

    result = 0xFEE00000 | (lapic_id << 12);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
