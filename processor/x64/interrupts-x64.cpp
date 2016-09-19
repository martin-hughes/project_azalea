// x64-specific interrupt handling code.

//#define ENABLE_TRACING

#include "processor/processor.h"
#include "klib/klib.h"

#include "processor/x64/processor-x64-int.h"
#include "processor/x64/pic/pic.h"

unsigned char interrupt_descriptor_table[NUM_INTERRUPTS * IDT_ENTRY_LEN];
void *end_of_irq_ack_fn = (void *)asm_proc_legacy_pic_irq_ack;

//------------------------------------------------------------------------------
// Basic interrupt handlers
//------------------------------------------------------------------------------

/// @brief Default interrupt handler
void proc_def_interrupt_handler()
{
  // Don't do anything just yet. Maybe one day!
  panic("Another interrupt called!");
}

/// @brief Interrupt handler that just panics
///
/// **Use this for testing only.**
void proc_panic_interrupt_handler()
{
  panic("Panic interrupt handler called.");
}

//------------------------------------------------------------------------------
// Interrupt system setup.
//------------------------------------------------------------------------------

/// @brief Configure the system's IDT
///
/// Configure the IDT on the BSP. This function is only meant to be called once, the APs simply copy the IDT.
void proc_configure_idt()
{
  static_assert(sizeof(long) == 8, "Type length check failed");
  static_assert(sizeof(int) == 4, "Type length check failed");
  static_assert(sizeof(short) == 2, "Type length check failed");

  // Start by zero-ing out everything.
  kl_memset(interrupt_descriptor_table, 0, sizeof(interrupt_descriptor_table));

  // Fill in the vectors we actually care about.
  for (int i = 0; i < 256; i++)
  {
    if ((i < 32) || (i > 48))
    {
      proc_configure_idt_entry(i, 0, (void *)asm_proc_def_interrupt_handler, 0);
    }
    else
    {
      proc_configure_idt_entry(i, 0, (void *)asm_proc_def_irq_handler, 0);
    }
  }
  proc_configure_idt_entry(0,  0, (void *)asm_proc_div_by_zero_fault_handler, 0);
  proc_configure_idt_entry(1,  0, (void *)asm_proc_debug_fault_handler, 0);
  proc_configure_idt_entry(2,  0, (void *)asm_proc_nmi_int_handler, 1);
  proc_configure_idt_entry(3,  0, (void *)asm_proc_brkpt_trap_handler, 0);
  proc_configure_idt_entry(4,  0, (void *)asm_proc_overflow_trap_handler, 0);
  proc_configure_idt_entry(5,  0, (void *)asm_proc_bound_range_fault_handler, 0);
  proc_configure_idt_entry(6,  0, (void *)asm_proc_invalid_opcode_fault_handler, 0);
  proc_configure_idt_entry(7,  0, (void *)asm_proc_device_not_avail_fault_handler, 0);
  proc_configure_idt_entry(8,  0, (void *)asm_proc_double_fault_abort_handler, 0);
  proc_configure_idt_entry(10, 0, (void *)asm_proc_invalid_tss_fault_handler, 0);
  proc_configure_idt_entry(11, 0, (void *)asm_proc_seg_not_present_fault_handler, 0);
  proc_configure_idt_entry(12, 0, (void *)asm_proc_ss_fault_handler, 0);
  proc_configure_idt_entry(13, 0, (void *)asm_proc_gen_prot_fault_handler, 0);
  proc_configure_idt_entry(14, 0, (void *)asm_proc_page_fault_handler, 0);
  proc_configure_idt_entry(16, 0, (void *)asm_proc_fp_except_fault_handler, 0);
  proc_configure_idt_entry(17, 0, (void *)asm_proc_align_check_fault_handler, 0);
  proc_configure_idt_entry(18, 0, (void *)asm_proc_machine_check_abort_handler, 0);
  proc_configure_idt_entry(19, 0, (void *)asm_proc_simd_fpe_fault_handler, 0);
  proc_configure_idt_entry(20, 0, (void *)asm_proc_virt_except_fault_handler, 0);
  proc_configure_idt_entry(30, 0, (void *)asm_proc_security_fault_handler, 0);
  proc_configure_idt_entry(49, 3, (void *)asm_proc_panic_interrupt_handler, 0);

  // Load the new IDT.
  asm_proc_install_idt();
}

/// @brief Fill in a single IDT entry
///
/// Fill in an entry of the IDT as an interrupt gate
///
/// @param interrupt_num The interrupt number that is to be configured.
///
/// @param req_priv_lvl Which privilege level is able to call this interrupt. Must be one of 0 or 3. Setting 0 means
///                     the interrupt can only be called by hardware, or from ring 0 when using the INT instruction.
///                     Setting 3 means user mode code can call the interrupt using INT.
///
/// @param fn_pointer Pointer to the function that ends up being called. This code must deal with preserving and
///                   restoring any relevant CPU state.
///
/// @param ist_num The Interrupt Stack Table number for the stack this interrupt handler uses. The system configures
///                entries 0 and 1, although 1 is not available until after the TSS is installed just prior to
///                beginning scheduling. If entries greater than 1 are used, the user is responsible for configuring
///                the TSS as appropriate.
void proc_configure_idt_entry(unsigned int interrupt_num, int req_priv_lvl, void *fn_pointer, unsigned char ist_num)
{
  KL_TRC_ENTRY;
  unsigned short low_bytes;
  unsigned short mid_bytes;
  unsigned int high_bytes;
  unsigned short segment_selector = 0x0008;
  unsigned short type_field = 0x8F00;

  ASSERT((req_priv_lvl == 0) || (req_priv_lvl == 3))
  if (req_priv_lvl)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Access from privilege level 3 requested\n");
    type_field |= 0x6000;
  }

  ASSERT(ist_num < 8);
  type_field = type_field | ist_num;

  unsigned long vector = (unsigned long)fn_pointer;
  low_bytes = (unsigned short)(vector & 0xFFFF);
  mid_bytes = (unsigned short)((vector & 0xFFFF0000) >> 16);
  high_bytes = (unsigned int)((vector & 0xFFFFFFFF00000000) >> 32);

  unsigned char *field_base = interrupt_descriptor_table + (interrupt_num *
      IDT_ENTRY_LEN);
  unsigned short *work_ptr_a = (unsigned short*)field_base;
  *work_ptr_a = low_bytes;
  work_ptr_a++;
  *work_ptr_a = segment_selector;
  work_ptr_a++;
  *work_ptr_a = type_field;
  work_ptr_a++;
  *work_ptr_a = mid_bytes;
  work_ptr_a++;

  ASSERT((unsigned char*)work_ptr_a == field_base + 8);

  unsigned int *work_ptr_b = (unsigned int *)work_ptr_a;
  *work_ptr_b = high_bytes;

  KL_TRC_EXIT;
}

/// @brief Handles page faults
///
/// Proper docs to follow when the system makes actual use of page faults.
///
/// @param fault_code See the Intel manual for more
/// @param fault_addr See the Intel manual for more
/// @param fault_instruction See the Intel manual for more
void proc_page_fault_handler(unsigned long fault_code, unsigned long fault_addr, unsigned long fault_instruction)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "fault code: ", fault_code, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "CR2 (bad mem address): ", fault_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Instruction pointer: ", fault_instruction, "\n");
  KL_TRC_EXIT;
  panic("Page fault!");
}
