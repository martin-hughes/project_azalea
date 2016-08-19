/// @file
/// @brief x64-specific processor management code.

//#define ENABLE_TRACING

#include "processor/processor.h"
#include "klib/klib.h"

#include "processor/x64/processor-x64.h"
#include "processor/x64/processor-x64-int.h"
#include "processor/x64/pic/pic.h"
#include "mem/x64/mem-x64-int.h"

unsigned char *tss_segment;
const unsigned char TSS_SEG_LENGTH = 104;

/// @brief Initialise the first processor.
///
/// Does as much initialisation of the BSP as possible. We leave some of the harder stuff, like configuring the APIC
/// until after the memory manager is running.
void proc_gen_init()
{
	// Interrupts should have been left disabled by the bootloader, but since
	// we're about to fiddle with the GDT, IDT and such, it's probably best
	// to make sure.
	asm_proc_stop_interrupts();

	// Fill in the GDT, and select an appropriate set of segments. The TSS descriptor and segment will
	// come later.
	asm_proc_load_gdt();
	tss_segment = (unsigned char *)NULL;

	// Fill in the IDT now, so we at least handle our own exceptions.
	proc_configure_idt();

	// Further processor setup, including configuring PICs/APICs, continues after the memory mamanger is up.
}

// TODO: There needs to be one TSS / TSS descriptor per processor. (MT)
/// @brief Create a TSS for the BSP
///
/// Create a TSS and configure the TSS descriptor in the GDT to point at it. This function is the the BSP only.
void proc_init_tss()
{
  KL_TRC_ENTRY;

  unsigned long tss_seg_ulong;

  // We make the assumption below that the length fits into one byte of the limit field.
  static_assert(TSS_SEG_LENGTH < 255, "The TSS segment size can't be described to the CPU");

  // Allocate a new TSS segment.
  tss_segment = new unsigned char[TSS_SEG_LENGTH];
  unsigned long *segment_rsp0;
  tss_seg_ulong = (unsigned long)tss_segment;
  kl_memset(tss_segment, 0, TSS_SEG_LENGTH);

  ////////////////////////////////////
  // Fill in TSS segment descriptor //
  ////////////////////////////////////

  KL_TRC_DATA("Filling in TSS GDT entry at", (unsigned long)tss_gdt_entry);
  KL_TRC_DATA("To point at TSS at", (unsigned long)tss_segment);

  // These two bytes define the length of the segment.
  tss_gdt_entry[0] = (unsigned char)(TSS_SEG_LENGTH - 1);
  tss_gdt_entry[1] = 0;

  // The next three bytes define the lowest three bytes of the base of the segment.
  tss_gdt_entry[2] = (unsigned char)(tss_seg_ulong  & 0x00000000000000FF);
  tss_gdt_entry[3] = (unsigned char)((tss_seg_ulong & 0x000000000000FF00) >> 8);
  tss_gdt_entry[4] = (unsigned char)((tss_seg_ulong & 0x0000000000FF0000) >> 16);

  // The next byte is formed of these bits (MSb first)
  // 1 - Present
  // 00 - DPL, but we don't want this called directly from ring 3.
  // 0 - As defined.
  // 1001 - Type, without busy (0x2) set.
  tss_gdt_entry[5] = 0x89;

  // The next byte is formed of these bits (MSb first)
  // 0 - Granularity of 1 byte.
  // 00 - as defined.
  // 1 - Available
  // 0000 - Highest nybble of the segment length.
  tss_gdt_entry[6] = 0x10;

  // The remaining 5 bytes of base
  tss_gdt_entry[7] =  (unsigned char)((tss_seg_ulong & 0x00000000FF000000) >> 24);
  tss_gdt_entry[8] =  (unsigned char)((tss_seg_ulong & 0x000000FF00000000) >> 32);
  tss_gdt_entry[9] =  (unsigned char)((tss_seg_ulong & 0x0000FF0000000000) >> 40);
  tss_gdt_entry[10] = (unsigned char)((tss_seg_ulong & 0x00FF000000000000) >> 48);
  tss_gdt_entry[11] = (unsigned char)((tss_seg_ulong & 0xFF00000000000000) >> 56);

  // Remaining 4 bytes are 0 or unused.
  tss_gdt_entry[12] = 0;
  tss_gdt_entry[13] = 0;
  tss_gdt_entry[14] = 0;
  tss_gdt_entry[15] = 0;

  /////////////////////////
  // Fill in TSS segment //
  /////////////////////////

  // The only important field is RSP0 - the stack pointer to use when jumping to ring 0.
  segment_rsp0 = (unsigned long *)(tss_segment + 4);
  *segment_rsp0 = (unsigned long)mem_x64_kernel_stack_ptr;
  KL_TRC_DATA("Set Kernel RSP to", (unsigned long)mem_x64_kernel_stack_ptr);
  KL_TRC_DATA("At position", (unsigned long)segment_rsp0);

  ///////////////////
  // Load the TSS! //
  ///////////////////
  KL_TRC_TRACE((TRC_LVL_FLOW, "About to load TSS\n"));
  asm_proc_load_gdt();
  asm_proc_load_tss();

  KL_TRC_EXIT;
}

/// @brief Cause this processor to enter the halted state.
void proc_stop_this_proc()
{
	asm_proc_stop_this_proc();
}

/// @brief Stop interrupts.
///
///This should only be used when preparing to panic, to prevent any race conditions.
void proc_stop_interrupts()
{
	asm_proc_stop_interrupts();
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

  KL_TRC_DATA("Reading MSR", msr);
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

  KL_TRC_DATA("Writing MSR", msr);
  KL_TRC_DATA("Value", value);

  asm_proc_write_msr(msr_l, value);

  KL_TRC_EXIT;
}
