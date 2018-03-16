/// @file
/// @brief x64-specific processor management code.

//#define ENABLE_TRACING

#include "processor/processor.h"
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
unsigned long proc_read_port(const unsigned long port_id, const unsigned char width)
{
  KL_TRC_ENTRY;

  unsigned long retval;

  KL_TRC_DATA("Port", port_id);
  KL_TRC_DATA("Width", width);

  ASSERT((width == 8) || (width == 16) || (width == 32));

  retval = asm_proc_read_port(port_id, width);

  KL_TRC_DATA("Returned value", retval);
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
void proc_write_port(const unsigned long port_id, const unsigned long value, const unsigned char width)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Port", port_id);
  KL_TRC_DATA("Value", value);
  KL_TRC_DATA("Width", width);

  ASSERT((width == 8) || (width == 16) || (width == 32));

  asm_proc_write_port(port_id, value, width);

  KL_TRC_EXIT;
}

/// @brief Read from a processor MSR
///
/// @param msr The MSR to read from.
///
/// @return The value of the MSR, combined in to a single 64 bit form.
unsigned long proc_read_msr(PROC_X64_MSRS msr)
{
  KL_TRC_ENTRY;

  unsigned long retval;
  unsigned long msr_l = static_cast<unsigned long>(msr);

  KL_TRC_DATA("Reading MSR", msr_l);
  retval = asm_proc_read_msr(msr_l);
  KL_TRC_DATA("Returned value", retval);

  KL_TRC_EXIT;

  return retval;
}

/// @brief Write to a processor MSR
///
/// @param msr The MSR to write to
///
/// @param value The 64-bit value to write out.
void proc_write_msr(PROC_X64_MSRS msr, unsigned long value)
{
  KL_TRC_ENTRY;

  unsigned long msr_l = static_cast<unsigned long>(msr);

  KL_TRC_DATA("Writing MSR", msr_l);
  KL_TRC_DATA("Value", value);

  asm_proc_write_msr(msr_l, value);

  KL_TRC_EXIT;
}

/// @brief Allocate a single-page stack to the kernel.
///
/// @return An address that can be used as a stack pointer, growing downwards as far as the next page boundary.
void *proc_x64_allocate_stack()
{
  KL_TRC_ENTRY;

  void *new_stack = kmalloc(MEM_PAGE_SIZE);
  new_stack = reinterpret_cast<void *>(reinterpret_cast<unsigned long>(new_stack) + MEM_PAGE_SIZE - 16);
  ASSERT((reinterpret_cast<unsigned long>(new_stack) & 0x0F) == 0);

  KL_TRC_DATA("Issuing new stack", reinterpret_cast<unsigned long>(new_stack));

  KL_TRC_EXIT;

  return new_stack;
}
