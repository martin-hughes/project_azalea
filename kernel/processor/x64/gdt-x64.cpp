/// @file
/// @brief Code to interact with the processors' GDTs

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/x64/processor-x64.h"
#include "processor/x64/processor-x64-int.h"
#include "processor/x64/pic/pic.h"
#include "mem/x64/mem-x64-int.h"

unsigned char *system_gdt = nullptr;
extern "C" unsigned char main_gdt_pointer[10];
extern "C" unsigned char initial_gdt_table;
extern "C" unsigned char initial_end_of_gdt_table;

const unsigned short gdt_entry_len = 16;
const unsigned char TSS_SEG_LENGTH = 104;

unsigned short proc_gdt_calc_req_len(unsigned int num_procs);
void proc_gdt_populate_pointer(unsigned char *ptr, unsigned long loc, unsigned short len);
void proc_generate_tss(unsigned char *tss_descriptor, void *kernel_stack_loc, void *ist1_stack_loc);
unsigned short proc_calc_tss_desc_offset(unsigned int proc_num);

/// @brief Recreate the GDT
///
/// Recreate the GDT in a location where it can be as long as needed - during startup, it is fixed in place, surrounded
/// by assembly instructions, so it is not possible to append one TSS descriptor per processor. As part of recreating
/// it, allocate enough space for all those TSS descriptors.
///
/// @param num_procs The number of processors in the system.
void proc_recreate_gdt(unsigned int num_procs)
{
  KL_TRC_ENTRY;

  unsigned short length_of_gdt;
  unsigned short initial_gdt_len;
  unsigned long gdt_loc;
  unsigned short offset;

  KL_TRC_DATA("Number of processors to create for", num_procs);

  ASSERT(num_procs > 0);

  // Assume this code is only called on the BSP.
  ASSERT(proc_mp_this_proc_id() == 0);

  initial_gdt_len = (unsigned short)(reinterpret_cast<unsigned long>(&initial_end_of_gdt_table) -
                                     reinterpret_cast<unsigned long>(&initial_gdt_table));

  KL_TRC_TRACE(TRC_LVL::FLOW, "More processors\n");
  length_of_gdt = proc_gdt_calc_req_len(num_procs);

  system_gdt = new unsigned char[length_of_gdt];

  ASSERT(length_of_gdt >= initial_gdt_len);

  kl_memcpy(&initial_gdt_table, system_gdt, initial_gdt_len);

  // Populate all the other TSSs
  for (int i = 0; i < num_procs; i++)
  {
    offset = proc_calc_tss_desc_offset(i);
    proc_generate_tss(&system_gdt[offset], proc_x64_allocate_stack(), proc_x64_allocate_stack());
  }

  proc_gdt_populate_pointer(main_gdt_pointer, reinterpret_cast<unsigned long>(system_gdt), length_of_gdt);
  asm_proc_load_gdt();

  KL_TRC_EXIT;
}

/// @brief Fills in a 6-byte GDT pointer table.
///
/// @param ptr The GDT pointer table to populate
///
/// @param loc The location of the GDT in virtual memory
///
/// @param len The length of the GDT.
void proc_gdt_populate_pointer(unsigned char *ptr, unsigned long loc, unsigned short len)
{
  KL_TRC_ENTRY;

  unsigned short *len_ptr = reinterpret_cast<unsigned short *>(ptr);
  unsigned long *loc_ptr = reinterpret_cast<unsigned long *>(ptr + 2);

  KL_TRC_DATA("Output pointer", reinterpret_cast<unsigned long>(ptr));
  KL_TRC_DATA("Location pointer", reinterpret_cast<unsigned long>(loc_ptr));
  KL_TRC_DATA("Location", loc);
  KL_TRC_DATA("Length", len);

  *len_ptr = len - 1;
  *loc_ptr = loc;

  KL_TRC_EXIT;
}

/// @brief Calculate the required length of the GDT for this system.
///
/// @param num_procs The number of processors in the system.
///
/// @return The required length of the GDT, in bytes.
unsigned short proc_gdt_calc_req_len(unsigned int num_procs)
{
  KL_TRC_ENTRY;

  // The length of the GDT is comprised of two parts:
  // 48 bytes of code and data segment descriptors (6 lots, see gdt-low-x64.asm for the details)
  // 16 bytes per processor of TSS descriptors.
  unsigned short result = 48 + (num_procs * 16);

  KL_TRC_DATA("Result", result);
  KL_TRC_EXIT;

  return result;
}

/// @brief Create a TSS for the BSP
///
/// Create a TSS and configure the TSS descriptor in the GDT to point at it. This function is for the BSP only.
void proc_init_tss()
{
  KL_TRC_ENTRY;

  proc_generate_tss(tss_gdt_entry,  mem_x64_kernel_stack_ptr, nullptr);

  KL_TRC_TRACE(TRC_LVL::FLOW, "About to load TSS\n");
  asm_proc_load_gdt();
  proc_load_tss(0);

  KL_TRC_EXIT;
}

/// @brief Create a new TSS and fill in its descriptor to point at it.
///
/// This function allocates a new Task State Segment and fills in the TSS descriptor with its location and size. It
/// also fills in the interesting fields of the TSS itself.
///
/// @param tss_descriptor A pointer to the TSS descriptor to populate
///
/// @param kernel_stack_loc A pointer to the kernel stack to use when entering the task described by this TSS.
///
/// @param ist1_stack_loc A pointer to a stack to use for interrupts using entry 1 of the Interrupt Stack Table
///                       mechanism. Initially, this is just the NMI handler.
void proc_generate_tss(unsigned char *tss_descriptor, void *kernel_stack_loc, void *ist1_stack_loc)
{
  KL_TRC_ENTRY;

  unsigned long tss_seg_ulong;
  unsigned char *tss_segment;
  unsigned long *ist1_entry;

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
  tss_descriptor[0] = (unsigned char)(TSS_SEG_LENGTH - 1);
  tss_descriptor[1] = 0;

  // The next three bytes define the lowest three bytes of the base of the segment.
  tss_descriptor[2] = (unsigned char)(tss_seg_ulong  & 0x00000000000000FF);
  tss_descriptor[3] = (unsigned char)((tss_seg_ulong & 0x000000000000FF00) >> 8);
  tss_descriptor[4] = (unsigned char)((tss_seg_ulong & 0x0000000000FF0000) >> 16);

  // The next byte is formed of these bits (MSb first)
  // 1 - Present
  // 00 - DPL, but we don't want this called directly from ring 3.
  // 0 - As defined.
  // 1001 - Type, without busy (0x2) set.
  tss_descriptor[5] = 0x89;

  // The next byte is formed of these bits (MSb first)
  // 0 - Granularity of 1 byte.
  // 00 - as defined.
  // 1 - Available
  // 0000 - Highest nybble of the segment length.
  tss_descriptor[6] = 0x10;

  // The remaining 5 bytes of base
  tss_descriptor[7] =  (unsigned char)((tss_seg_ulong & 0x00000000FF000000) >> 24);
  tss_descriptor[8] =  (unsigned char)((tss_seg_ulong & 0x000000FF00000000) >> 32);
  tss_descriptor[9] =  (unsigned char)((tss_seg_ulong & 0x0000FF0000000000) >> 40);
  tss_descriptor[10] = (unsigned char)((tss_seg_ulong & 0x00FF000000000000) >> 48);
  tss_descriptor[11] = (unsigned char)((tss_seg_ulong & 0xFF00000000000000) >> 56);

  // Remaining 4 bytes are 0 or unused.
  tss_descriptor[12] = 0;
  tss_descriptor[13] = 0;
  tss_descriptor[14] = 0;
  tss_descriptor[15] = 0;

  /////////////////////////
  // Fill in TSS segment //
  /////////////////////////

  // There are two important fields. The first is RSP0 - the stack pointer to use when jumping to ring 0.
  segment_rsp0 = reinterpret_cast<unsigned long *>(tss_segment + 4);
  *segment_rsp0 = reinterpret_cast<unsigned long>(kernel_stack_loc);
  KL_TRC_DATA("Set Kernel RSP to", (unsigned long)kernel_stack_loc);
  KL_TRC_DATA("At position", (unsigned long)segment_rsp0);

  // Next, fill in IST1
  ist1_entry = reinterpret_cast<unsigned long *>(tss_segment + 36);
  *ist1_entry = reinterpret_cast<unsigned long>(ist1_stack_loc);

  KL_TRC_EXIT;
}

void proc_load_tss(unsigned int proc_num)
{
  KL_TRC_ENTRY;

  unsigned long offset = proc_calc_tss_desc_offset(proc_num);

  KL_TRC_DATA("Processor number", proc_num);
  KL_TRC_DATA("Offset", offset);

  asm_proc_load_tss(offset);

  KL_TRC_EXIT;
}

/// @brief Given a processor, calculate the offset within the GDT of its TSS descriptor
///
/// @param proc_num The ID number of the desired processor
unsigned short proc_calc_tss_desc_offset(unsigned int proc_num)
{
  KL_TRC_ENTRY;

  unsigned short result;

  // Reasoning is as per proc_gdt_calc_req_len
  result = 48 + (16 * proc_num);

  KL_TRC_DATA("Result", result);
  KL_TRC_EXIT;

  return result;
}
