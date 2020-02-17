/// @file
/// @brief x64-specific interrupt handling code.

//#define ENABLE_TRACING

#include "processor/processor.h"
#include "processor/processor-int.h"
#include "klib/klib.h"

#include "processor/x64/processor-x64-int.h"
#include "processor/x64/proc_interrupt_handlers-x64.h"
#include "processor/x64/pic/pic.h"

/// Storage for the system IDT.
///
uint8_t interrupt_descriptor_table[NUM_INTERRUPTS * IDT_ENTRY_LEN];

/// @brief Function to be called when an IRQ has been handled.
///
void *end_of_irq_ack_fn = (void *)asm_proc_legacy_pic_irq_ack;

/// @brief How many interrupts does this system support?
///
const uint16_t PROC_NUM_INTERRUPTS = 256;

/// @brief How many interrupts are given over to IRQs?
///
const uint16_t PROC_NUM_IRQS = 16;

/// @brief The IRQ handlers are in a contiguous batch starting at which interrupt number?
///
const uint16_t PROC_IRQ_BASE = 32;

/// @brief Generic information about interrupt handlers.
///
/// Some handlers are processor-specific, in which case they are marked in this table as reserved.
proc_interrupt_data proc_interrupt_data_table[PROC_NUM_INTERRUPTS];

namespace
{
  void proc_x64_config_plain_handlers();
}

//------------------------------------------------------------------------------
// Interrupt system setup.
//------------------------------------------------------------------------------

/// @brief Configure the system's IDT
///
/// Configure the IDT on the BSP. This function is only meant to be called once, the APs simply copy the IDT.
void proc_configure_idt()
{
  // Start by zero-ing out everything.
  memset(interrupt_descriptor_table, 0, sizeof(interrupt_descriptor_table));
  proc_x64_config_plain_handlers();

  // Configure the Intel-defined exception handlers.
  proc_configure_idt_entry(0,  0, (void *)asm_proc_div_by_zero_fault_handler, 1);
  proc_configure_idt_entry(1,  0, (void *)asm_proc_debug_fault_handler, 1);
  proc_configure_idt_entry(2,  0, (void *)asm_proc_nmi_int_handler, 2);
  proc_configure_idt_entry(3,  0, (void *)asm_proc_brkpt_trap_handler, 1);
  proc_configure_idt_entry(4,  0, (void *)asm_proc_overflow_trap_handler, 1);
  proc_configure_idt_entry(5,  0, (void *)asm_proc_bound_range_fault_handler, 1);
  proc_configure_idt_entry(6,  0, (void *)asm_proc_invalid_opcode_fault_handler, 1);
  proc_configure_idt_entry(7,  0, (void *)asm_proc_device_not_avail_fault_handler, 1);
  proc_configure_idt_entry(8,  0, (void *)asm_proc_double_fault_abort_handler, 1);
  proc_configure_idt_entry(10, 0, (void *)asm_proc_invalid_tss_fault_handler, 1);
  proc_configure_idt_entry(11, 0, (void *)asm_proc_seg_not_present_fault_handler, 1);
  proc_configure_idt_entry(12, 0, (void *)asm_proc_ss_fault_handler, 1);
  proc_configure_idt_entry(13, 0, (void *)asm_proc_gen_prot_fault_handler, 1);
  proc_configure_idt_entry(14, 0, (void *)asm_proc_page_fault_handler, 1);
  proc_configure_idt_entry(16, 0, (void *)asm_proc_fp_except_fault_handler, 1);
  proc_configure_idt_entry(17, 0, (void *)asm_proc_align_check_fault_handler, 1);
  proc_configure_idt_entry(18, 0, (void *)asm_proc_machine_check_abort_handler, 1);
  proc_configure_idt_entry(19, 0, (void *)asm_proc_simd_fpe_fault_handler, 1);
  proc_configure_idt_entry(20, 0, (void *)asm_proc_virt_except_fault_handler, 1);
  proc_configure_idt_entry(30, 0, (void *)asm_proc_security_fault_handler, 1);

  // Configure a bunch of IRQ handlers.
  proc_configure_idt_entry(32, 0, (void *)asm_proc_handle_irq_0, 1);
  proc_configure_idt_entry(33, 0, (void *)asm_proc_handle_irq_1, 1);
  proc_configure_idt_entry(34, 0, (void *)asm_proc_handle_irq_2, 1);
  proc_configure_idt_entry(35, 0, (void *)asm_proc_handle_irq_3, 1);
  proc_configure_idt_entry(36, 0, (void *)asm_proc_handle_irq_4, 1);
  proc_configure_idt_entry(37, 0, (void *)asm_proc_handle_irq_5, 1);
  proc_configure_idt_entry(38, 0, (void *)asm_proc_handle_irq_6, 1);
  proc_configure_idt_entry(39, 0, (void *)asm_proc_handle_irq_7, 1);
  proc_configure_idt_entry(40, 0, (void *)asm_proc_handle_irq_8, 1);
  proc_configure_idt_entry(41, 0, (void *)asm_proc_handle_irq_9, 1);
  proc_configure_idt_entry(42, 0, (void *)asm_proc_handle_irq_10, 1);
  proc_configure_idt_entry(43, 0, (void *)asm_proc_handle_irq_11, 1);
  proc_configure_idt_entry(44, 0, (void *)asm_proc_handle_irq_12, 1);
  proc_configure_idt_entry(45, 0, (void *)asm_proc_handle_irq_13, 1);
  proc_configure_idt_entry(46, 0, (void *)asm_proc_handle_irq_14, 1);
  proc_configure_idt_entry(47, 0, (void *)asm_proc_handle_irq_15, 1);

  // Make sure the system considers all of the processor-specific and IRQ handlers as reserved, so that no driver can
  // try to register a handler for them.
  for (int i = 0; i < (PROC_IRQ_BASE + PROC_NUM_IRQS); i++)
  {
    proc_interrupt_data_table[i].reserved = true;

    if (i >= PROC_IRQ_BASE)
    {
      proc_interrupt_data_table[i].is_irq = true;
    }
  }

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
///                entries 1, 2, and 3 as described in the comments for proc_generate_tss(). Since the system uses the
///                red zone defined by the AMD x64 ABI it is mandatory for all interrupts to use the IST mechanism - so
///                valid values of ist_num are 1-7 inclusive (note - 4-7 inclusive are not configured for use, the
///                caller would be responsible for this.)
void proc_configure_idt_entry(uint32_t interrupt_num, int req_priv_lvl, void *fn_pointer, uint8_t ist_num)
{
  KL_TRC_ENTRY;
  uint16_t low_bytes;
  uint16_t mid_bytes;
  uint32_t high_bytes;
  uint16_t segment_selector = 0x0008;
  uint16_t type_field = 0x8F00;

  ASSERT((req_priv_lvl == 0) || (req_priv_lvl == 3))
  if (req_priv_lvl)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Access from privilege level 3 requested\n");
    type_field |= 0x6000;
  }

  ASSERT((ist_num > 0) && (ist_num < 8));
  type_field = type_field | ist_num;

  uint64_t vector = (uint64_t)fn_pointer;
  low_bytes = (uint16_t)(vector & 0xFFFF);
  mid_bytes = (uint16_t)((vector & 0xFFFF0000) >> 16);
  high_bytes = (uint32_t)((vector & 0xFFFFFFFF00000000) >> 32);

  uint8_t *field_base = interrupt_descriptor_table + (interrupt_num *
      IDT_ENTRY_LEN);
  uint16_t *work_ptr_a = (uint16_t*)field_base;
  *work_ptr_a = low_bytes;
  work_ptr_a++;
  *work_ptr_a = segment_selector;
  work_ptr_a++;
  *work_ptr_a = type_field;
  work_ptr_a++;
  *work_ptr_a = mid_bytes;
  work_ptr_a++;

  ASSERT((uint8_t*)work_ptr_a == field_base + 8);

  uint32_t *work_ptr_b = (uint32_t *)work_ptr_a;
  *work_ptr_b = high_bytes;

  KL_TRC_EXIT;
}

namespace
{
  /// @brief Configures a whole IDT of interrupt handlers that simply call the default handler with a parameter.
  void proc_x64_config_plain_handlers()
  {
    proc_configure_idt_entry(0, 0, (void *)asm_proc_interrupt_0_handler, 1);
    proc_configure_idt_entry(1, 0, (void *)asm_proc_interrupt_1_handler, 1);
    proc_configure_idt_entry(2, 0, (void *)asm_proc_interrupt_2_handler, 1);
    proc_configure_idt_entry(3, 0, (void *)asm_proc_interrupt_3_handler, 1);
    proc_configure_idt_entry(4, 0, (void *)asm_proc_interrupt_4_handler, 1);
    proc_configure_idt_entry(5, 0, (void *)asm_proc_interrupt_5_handler, 1);
    proc_configure_idt_entry(6, 0, (void *)asm_proc_interrupt_6_handler, 1);
    proc_configure_idt_entry(7, 0, (void *)asm_proc_interrupt_7_handler, 1);
    proc_configure_idt_entry(8, 0, (void *)asm_proc_interrupt_8_handler, 1);
    proc_configure_idt_entry(9, 0, (void *)asm_proc_interrupt_9_handler, 1);
    proc_configure_idt_entry(10, 0, (void *)asm_proc_interrupt_10_handler, 1);
    proc_configure_idt_entry(11, 0, (void *)asm_proc_interrupt_11_handler, 1);
    proc_configure_idt_entry(12, 0, (void *)asm_proc_interrupt_12_handler, 1);
    proc_configure_idt_entry(13, 0, (void *)asm_proc_interrupt_13_handler, 1);
    proc_configure_idt_entry(14, 0, (void *)asm_proc_interrupt_14_handler, 1);
    proc_configure_idt_entry(15, 0, (void *)asm_proc_interrupt_15_handler, 1);
    proc_configure_idt_entry(16, 0, (void *)asm_proc_interrupt_16_handler, 1);
    proc_configure_idt_entry(17, 0, (void *)asm_proc_interrupt_17_handler, 1);
    proc_configure_idt_entry(18, 0, (void *)asm_proc_interrupt_18_handler, 1);
    proc_configure_idt_entry(19, 0, (void *)asm_proc_interrupt_19_handler, 1);
    proc_configure_idt_entry(20, 0, (void *)asm_proc_interrupt_20_handler, 1);
    proc_configure_idt_entry(21, 0, (void *)asm_proc_interrupt_21_handler, 1);
    proc_configure_idt_entry(22, 0, (void *)asm_proc_interrupt_22_handler, 1);
    proc_configure_idt_entry(23, 0, (void *)asm_proc_interrupt_23_handler, 1);
    proc_configure_idt_entry(24, 0, (void *)asm_proc_interrupt_24_handler, 1);
    proc_configure_idt_entry(25, 0, (void *)asm_proc_interrupt_25_handler, 1);
    proc_configure_idt_entry(26, 0, (void *)asm_proc_interrupt_26_handler, 1);
    proc_configure_idt_entry(27, 0, (void *)asm_proc_interrupt_27_handler, 1);
    proc_configure_idt_entry(28, 0, (void *)asm_proc_interrupt_28_handler, 1);
    proc_configure_idt_entry(29, 0, (void *)asm_proc_interrupt_29_handler, 1);
    proc_configure_idt_entry(30, 0, (void *)asm_proc_interrupt_30_handler, 1);
    proc_configure_idt_entry(31, 0, (void *)asm_proc_interrupt_31_handler, 1);
    proc_configure_idt_entry(32, 0, (void *)asm_proc_interrupt_32_handler, 1);
    proc_configure_idt_entry(33, 0, (void *)asm_proc_interrupt_33_handler, 1);
    proc_configure_idt_entry(34, 0, (void *)asm_proc_interrupt_34_handler, 1);
    proc_configure_idt_entry(35, 0, (void *)asm_proc_interrupt_35_handler, 1);
    proc_configure_idt_entry(36, 0, (void *)asm_proc_interrupt_36_handler, 1);
    proc_configure_idt_entry(37, 0, (void *)asm_proc_interrupt_37_handler, 1);
    proc_configure_idt_entry(38, 0, (void *)asm_proc_interrupt_38_handler, 1);
    proc_configure_idt_entry(39, 0, (void *)asm_proc_interrupt_39_handler, 1);
    proc_configure_idt_entry(40, 0, (void *)asm_proc_interrupt_40_handler, 1);
    proc_configure_idt_entry(41, 0, (void *)asm_proc_interrupt_41_handler, 1);
    proc_configure_idt_entry(42, 0, (void *)asm_proc_interrupt_42_handler, 1);
    proc_configure_idt_entry(43, 0, (void *)asm_proc_interrupt_43_handler, 1);
    proc_configure_idt_entry(44, 0, (void *)asm_proc_interrupt_44_handler, 1);
    proc_configure_idt_entry(45, 0, (void *)asm_proc_interrupt_45_handler, 1);
    proc_configure_idt_entry(46, 0, (void *)asm_proc_interrupt_46_handler, 1);
    proc_configure_idt_entry(47, 0, (void *)asm_proc_interrupt_47_handler, 1);
    proc_configure_idt_entry(48, 0, (void *)asm_proc_interrupt_48_handler, 1);
    proc_configure_idt_entry(49, 0, (void *)asm_proc_interrupt_49_handler, 1);
    proc_configure_idt_entry(50, 0, (void *)asm_proc_interrupt_50_handler, 1);
    proc_configure_idt_entry(51, 0, (void *)asm_proc_interrupt_51_handler, 1);
    proc_configure_idt_entry(52, 0, (void *)asm_proc_interrupt_52_handler, 1);
    proc_configure_idt_entry(53, 0, (void *)asm_proc_interrupt_53_handler, 1);
    proc_configure_idt_entry(54, 0, (void *)asm_proc_interrupt_54_handler, 1);
    proc_configure_idt_entry(55, 0, (void *)asm_proc_interrupt_55_handler, 1);
    proc_configure_idt_entry(56, 0, (void *)asm_proc_interrupt_56_handler, 1);
    proc_configure_idt_entry(57, 0, (void *)asm_proc_interrupt_57_handler, 1);
    proc_configure_idt_entry(58, 0, (void *)asm_proc_interrupt_58_handler, 1);
    proc_configure_idt_entry(59, 0, (void *)asm_proc_interrupt_59_handler, 1);
    proc_configure_idt_entry(60, 0, (void *)asm_proc_interrupt_60_handler, 1);
    proc_configure_idt_entry(61, 0, (void *)asm_proc_interrupt_61_handler, 1);
    proc_configure_idt_entry(62, 0, (void *)asm_proc_interrupt_62_handler, 1);
    proc_configure_idt_entry(63, 0, (void *)asm_proc_interrupt_63_handler, 1);
    proc_configure_idt_entry(64, 0, (void *)asm_proc_interrupt_64_handler, 1);
    proc_configure_idt_entry(65, 0, (void *)asm_proc_interrupt_65_handler, 1);
    proc_configure_idt_entry(66, 0, (void *)asm_proc_interrupt_66_handler, 1);
    proc_configure_idt_entry(67, 0, (void *)asm_proc_interrupt_67_handler, 1);
    proc_configure_idt_entry(68, 0, (void *)asm_proc_interrupt_68_handler, 1);
    proc_configure_idt_entry(69, 0, (void *)asm_proc_interrupt_69_handler, 1);
    proc_configure_idt_entry(70, 0, (void *)asm_proc_interrupt_70_handler, 1);
    proc_configure_idt_entry(71, 0, (void *)asm_proc_interrupt_71_handler, 1);
    proc_configure_idt_entry(72, 0, (void *)asm_proc_interrupt_72_handler, 1);
    proc_configure_idt_entry(73, 0, (void *)asm_proc_interrupt_73_handler, 1);
    proc_configure_idt_entry(74, 0, (void *)asm_proc_interrupt_74_handler, 1);
    proc_configure_idt_entry(75, 0, (void *)asm_proc_interrupt_75_handler, 1);
    proc_configure_idt_entry(76, 0, (void *)asm_proc_interrupt_76_handler, 1);
    proc_configure_idt_entry(77, 0, (void *)asm_proc_interrupt_77_handler, 1);
    proc_configure_idt_entry(78, 0, (void *)asm_proc_interrupt_78_handler, 1);
    proc_configure_idt_entry(79, 0, (void *)asm_proc_interrupt_79_handler, 1);
    proc_configure_idt_entry(80, 0, (void *)asm_proc_interrupt_80_handler, 1);
    proc_configure_idt_entry(81, 0, (void *)asm_proc_interrupt_81_handler, 1);
    proc_configure_idt_entry(82, 0, (void *)asm_proc_interrupt_82_handler, 1);
    proc_configure_idt_entry(83, 0, (void *)asm_proc_interrupt_83_handler, 1);
    proc_configure_idt_entry(84, 0, (void *)asm_proc_interrupt_84_handler, 1);
    proc_configure_idt_entry(85, 0, (void *)asm_proc_interrupt_85_handler, 1);
    proc_configure_idt_entry(86, 0, (void *)asm_proc_interrupt_86_handler, 1);
    proc_configure_idt_entry(87, 0, (void *)asm_proc_interrupt_87_handler, 1);
    proc_configure_idt_entry(88, 0, (void *)asm_proc_interrupt_88_handler, 1);
    proc_configure_idt_entry(89, 0, (void *)asm_proc_interrupt_89_handler, 1);
    proc_configure_idt_entry(90, 0, (void *)asm_proc_interrupt_90_handler, 1);
    proc_configure_idt_entry(91, 0, (void *)asm_proc_interrupt_91_handler, 1);
    proc_configure_idt_entry(92, 0, (void *)asm_proc_interrupt_92_handler, 1);
    proc_configure_idt_entry(93, 0, (void *)asm_proc_interrupt_93_handler, 1);
    proc_configure_idt_entry(94, 0, (void *)asm_proc_interrupt_94_handler, 1);
    proc_configure_idt_entry(95, 0, (void *)asm_proc_interrupt_95_handler, 1);
    proc_configure_idt_entry(96, 0, (void *)asm_proc_interrupt_96_handler, 1);
    proc_configure_idt_entry(97, 0, (void *)asm_proc_interrupt_97_handler, 1);
    proc_configure_idt_entry(98, 0, (void *)asm_proc_interrupt_98_handler, 1);
    proc_configure_idt_entry(99, 0, (void *)asm_proc_interrupt_99_handler, 1);
    proc_configure_idt_entry(100, 0, (void *)asm_proc_interrupt_100_handler, 1);
    proc_configure_idt_entry(101, 0, (void *)asm_proc_interrupt_101_handler, 1);
    proc_configure_idt_entry(102, 0, (void *)asm_proc_interrupt_102_handler, 1);
    proc_configure_idt_entry(103, 0, (void *)asm_proc_interrupt_103_handler, 1);
    proc_configure_idt_entry(104, 0, (void *)asm_proc_interrupt_104_handler, 1);
    proc_configure_idt_entry(105, 0, (void *)asm_proc_interrupt_105_handler, 1);
    proc_configure_idt_entry(106, 0, (void *)asm_proc_interrupt_106_handler, 1);
    proc_configure_idt_entry(107, 0, (void *)asm_proc_interrupt_107_handler, 1);
    proc_configure_idt_entry(108, 0, (void *)asm_proc_interrupt_108_handler, 1);
    proc_configure_idt_entry(109, 0, (void *)asm_proc_interrupt_109_handler, 1);
    proc_configure_idt_entry(110, 0, (void *)asm_proc_interrupt_110_handler, 1);
    proc_configure_idt_entry(111, 0, (void *)asm_proc_interrupt_111_handler, 1);
    proc_configure_idt_entry(112, 0, (void *)asm_proc_interrupt_112_handler, 1);
    proc_configure_idt_entry(113, 0, (void *)asm_proc_interrupt_113_handler, 1);
    proc_configure_idt_entry(114, 0, (void *)asm_proc_interrupt_114_handler, 1);
    proc_configure_idt_entry(115, 0, (void *)asm_proc_interrupt_115_handler, 1);
    proc_configure_idt_entry(116, 0, (void *)asm_proc_interrupt_116_handler, 1);
    proc_configure_idt_entry(117, 0, (void *)asm_proc_interrupt_117_handler, 1);
    proc_configure_idt_entry(118, 0, (void *)asm_proc_interrupt_118_handler, 1);
    proc_configure_idt_entry(119, 0, (void *)asm_proc_interrupt_119_handler, 1);
    proc_configure_idt_entry(120, 0, (void *)asm_proc_interrupt_120_handler, 1);
    proc_configure_idt_entry(121, 0, (void *)asm_proc_interrupt_121_handler, 1);
    proc_configure_idt_entry(122, 0, (void *)asm_proc_interrupt_122_handler, 1);
    proc_configure_idt_entry(123, 0, (void *)asm_proc_interrupt_123_handler, 1);
    proc_configure_idt_entry(124, 0, (void *)asm_proc_interrupt_124_handler, 1);
    proc_configure_idt_entry(125, 0, (void *)asm_proc_interrupt_125_handler, 1);
    proc_configure_idt_entry(126, 0, (void *)asm_proc_interrupt_126_handler, 1);
    proc_configure_idt_entry(127, 0, (void *)asm_proc_interrupt_127_handler, 1);
    proc_configure_idt_entry(128, 0, (void *)asm_proc_interrupt_128_handler, 1);
    proc_configure_idt_entry(129, 0, (void *)asm_proc_interrupt_129_handler, 1);
    proc_configure_idt_entry(130, 0, (void *)asm_proc_interrupt_130_handler, 1);
    proc_configure_idt_entry(131, 0, (void *)asm_proc_interrupt_131_handler, 1);
    proc_configure_idt_entry(132, 0, (void *)asm_proc_interrupt_132_handler, 1);
    proc_configure_idt_entry(133, 0, (void *)asm_proc_interrupt_133_handler, 1);
    proc_configure_idt_entry(134, 0, (void *)asm_proc_interrupt_134_handler, 1);
    proc_configure_idt_entry(135, 0, (void *)asm_proc_interrupt_135_handler, 1);
    proc_configure_idt_entry(136, 0, (void *)asm_proc_interrupt_136_handler, 1);
    proc_configure_idt_entry(137, 0, (void *)asm_proc_interrupt_137_handler, 1);
    proc_configure_idt_entry(138, 0, (void *)asm_proc_interrupt_138_handler, 1);
    proc_configure_idt_entry(139, 0, (void *)asm_proc_interrupt_139_handler, 1);
    proc_configure_idt_entry(140, 0, (void *)asm_proc_interrupt_140_handler, 1);
    proc_configure_idt_entry(141, 0, (void *)asm_proc_interrupt_141_handler, 1);
    proc_configure_idt_entry(142, 0, (void *)asm_proc_interrupt_142_handler, 1);
    proc_configure_idt_entry(143, 0, (void *)asm_proc_interrupt_143_handler, 1);
    proc_configure_idt_entry(144, 0, (void *)asm_proc_interrupt_144_handler, 1);
    proc_configure_idt_entry(145, 0, (void *)asm_proc_interrupt_145_handler, 1);
    proc_configure_idt_entry(146, 0, (void *)asm_proc_interrupt_146_handler, 1);
    proc_configure_idt_entry(147, 0, (void *)asm_proc_interrupt_147_handler, 1);
    proc_configure_idt_entry(148, 0, (void *)asm_proc_interrupt_148_handler, 1);
    proc_configure_idt_entry(149, 0, (void *)asm_proc_interrupt_149_handler, 1);
    proc_configure_idt_entry(150, 0, (void *)asm_proc_interrupt_150_handler, 1);
    proc_configure_idt_entry(151, 0, (void *)asm_proc_interrupt_151_handler, 1);
    proc_configure_idt_entry(152, 0, (void *)asm_proc_interrupt_152_handler, 1);
    proc_configure_idt_entry(153, 0, (void *)asm_proc_interrupt_153_handler, 1);
    proc_configure_idt_entry(154, 0, (void *)asm_proc_interrupt_154_handler, 1);
    proc_configure_idt_entry(155, 0, (void *)asm_proc_interrupt_155_handler, 1);
    proc_configure_idt_entry(156, 0, (void *)asm_proc_interrupt_156_handler, 1);
    proc_configure_idt_entry(157, 0, (void *)asm_proc_interrupt_157_handler, 1);
    proc_configure_idt_entry(158, 0, (void *)asm_proc_interrupt_158_handler, 1);
    proc_configure_idt_entry(159, 0, (void *)asm_proc_interrupt_159_handler, 1);
    proc_configure_idt_entry(160, 0, (void *)asm_proc_interrupt_160_handler, 1);
    proc_configure_idt_entry(161, 0, (void *)asm_proc_interrupt_161_handler, 1);
    proc_configure_idt_entry(162, 0, (void *)asm_proc_interrupt_162_handler, 1);
    proc_configure_idt_entry(163, 0, (void *)asm_proc_interrupt_163_handler, 1);
    proc_configure_idt_entry(164, 0, (void *)asm_proc_interrupt_164_handler, 1);
    proc_configure_idt_entry(165, 0, (void *)asm_proc_interrupt_165_handler, 1);
    proc_configure_idt_entry(166, 0, (void *)asm_proc_interrupt_166_handler, 1);
    proc_configure_idt_entry(167, 0, (void *)asm_proc_interrupt_167_handler, 1);
    proc_configure_idt_entry(168, 0, (void *)asm_proc_interrupt_168_handler, 1);
    proc_configure_idt_entry(169, 0, (void *)asm_proc_interrupt_169_handler, 1);
    proc_configure_idt_entry(170, 0, (void *)asm_proc_interrupt_170_handler, 1);
    proc_configure_idt_entry(171, 0, (void *)asm_proc_interrupt_171_handler, 1);
    proc_configure_idt_entry(172, 0, (void *)asm_proc_interrupt_172_handler, 1);
    proc_configure_idt_entry(173, 0, (void *)asm_proc_interrupt_173_handler, 1);
    proc_configure_idt_entry(174, 0, (void *)asm_proc_interrupt_174_handler, 1);
    proc_configure_idt_entry(175, 0, (void *)asm_proc_interrupt_175_handler, 1);
    proc_configure_idt_entry(176, 0, (void *)asm_proc_interrupt_176_handler, 1);
    proc_configure_idt_entry(177, 0, (void *)asm_proc_interrupt_177_handler, 1);
    proc_configure_idt_entry(178, 0, (void *)asm_proc_interrupt_178_handler, 1);
    proc_configure_idt_entry(179, 0, (void *)asm_proc_interrupt_179_handler, 1);
    proc_configure_idt_entry(180, 0, (void *)asm_proc_interrupt_180_handler, 1);
    proc_configure_idt_entry(181, 0, (void *)asm_proc_interrupt_181_handler, 1);
    proc_configure_idt_entry(182, 0, (void *)asm_proc_interrupt_182_handler, 1);
    proc_configure_idt_entry(183, 0, (void *)asm_proc_interrupt_183_handler, 1);
    proc_configure_idt_entry(184, 0, (void *)asm_proc_interrupt_184_handler, 1);
    proc_configure_idt_entry(185, 0, (void *)asm_proc_interrupt_185_handler, 1);
    proc_configure_idt_entry(186, 0, (void *)asm_proc_interrupt_186_handler, 1);
    proc_configure_idt_entry(187, 0, (void *)asm_proc_interrupt_187_handler, 1);
    proc_configure_idt_entry(188, 0, (void *)asm_proc_interrupt_188_handler, 1);
    proc_configure_idt_entry(189, 0, (void *)asm_proc_interrupt_189_handler, 1);
    proc_configure_idt_entry(190, 0, (void *)asm_proc_interrupt_190_handler, 1);
    proc_configure_idt_entry(191, 0, (void *)asm_proc_interrupt_191_handler, 1);
    proc_configure_idt_entry(192, 0, (void *)asm_proc_interrupt_192_handler, 1);
    proc_configure_idt_entry(193, 0, (void *)asm_proc_interrupt_193_handler, 1);
    proc_configure_idt_entry(194, 0, (void *)asm_proc_interrupt_194_handler, 1);
    proc_configure_idt_entry(195, 0, (void *)asm_proc_interrupt_195_handler, 1);
    proc_configure_idt_entry(196, 0, (void *)asm_proc_interrupt_196_handler, 1);
    proc_configure_idt_entry(197, 0, (void *)asm_proc_interrupt_197_handler, 1);
    proc_configure_idt_entry(198, 0, (void *)asm_proc_interrupt_198_handler, 1);
    proc_configure_idt_entry(199, 0, (void *)asm_proc_interrupt_199_handler, 1);
    proc_configure_idt_entry(200, 0, (void *)asm_proc_interrupt_200_handler, 1);
    proc_configure_idt_entry(201, 0, (void *)asm_proc_interrupt_201_handler, 1);
    proc_configure_idt_entry(202, 0, (void *)asm_proc_interrupt_202_handler, 1);
    proc_configure_idt_entry(203, 0, (void *)asm_proc_interrupt_203_handler, 1);
    proc_configure_idt_entry(204, 0, (void *)asm_proc_interrupt_204_handler, 1);
    proc_configure_idt_entry(205, 0, (void *)asm_proc_interrupt_205_handler, 1);
    proc_configure_idt_entry(206, 0, (void *)asm_proc_interrupt_206_handler, 1);
    proc_configure_idt_entry(207, 0, (void *)asm_proc_interrupt_207_handler, 1);
    proc_configure_idt_entry(208, 0, (void *)asm_proc_interrupt_208_handler, 1);
    proc_configure_idt_entry(209, 0, (void *)asm_proc_interrupt_209_handler, 1);
    proc_configure_idt_entry(210, 0, (void *)asm_proc_interrupt_210_handler, 1);
    proc_configure_idt_entry(211, 0, (void *)asm_proc_interrupt_211_handler, 1);
    proc_configure_idt_entry(212, 0, (void *)asm_proc_interrupt_212_handler, 1);
    proc_configure_idt_entry(213, 0, (void *)asm_proc_interrupt_213_handler, 1);
    proc_configure_idt_entry(214, 0, (void *)asm_proc_interrupt_214_handler, 1);
    proc_configure_idt_entry(215, 0, (void *)asm_proc_interrupt_215_handler, 1);
    proc_configure_idt_entry(216, 0, (void *)asm_proc_interrupt_216_handler, 1);
    proc_configure_idt_entry(217, 0, (void *)asm_proc_interrupt_217_handler, 1);
    proc_configure_idt_entry(218, 0, (void *)asm_proc_interrupt_218_handler, 1);
    proc_configure_idt_entry(219, 0, (void *)asm_proc_interrupt_219_handler, 1);
    proc_configure_idt_entry(220, 0, (void *)asm_proc_interrupt_220_handler, 1);
    proc_configure_idt_entry(221, 0, (void *)asm_proc_interrupt_221_handler, 1);
    proc_configure_idt_entry(222, 0, (void *)asm_proc_interrupt_222_handler, 1);
    proc_configure_idt_entry(223, 0, (void *)asm_proc_interrupt_223_handler, 1);
    proc_configure_idt_entry(224, 0, (void *)asm_proc_interrupt_224_handler, 1);
    proc_configure_idt_entry(225, 0, (void *)asm_proc_interrupt_225_handler, 1);
    proc_configure_idt_entry(226, 0, (void *)asm_proc_interrupt_226_handler, 1);
    proc_configure_idt_entry(227, 0, (void *)asm_proc_interrupt_227_handler, 1);
    proc_configure_idt_entry(228, 0, (void *)asm_proc_interrupt_228_handler, 1);
    proc_configure_idt_entry(229, 0, (void *)asm_proc_interrupt_229_handler, 1);
    proc_configure_idt_entry(230, 0, (void *)asm_proc_interrupt_230_handler, 1);
    proc_configure_idt_entry(231, 0, (void *)asm_proc_interrupt_231_handler, 1);
    proc_configure_idt_entry(232, 0, (void *)asm_proc_interrupt_232_handler, 1);
    proc_configure_idt_entry(233, 0, (void *)asm_proc_interrupt_233_handler, 1);
    proc_configure_idt_entry(234, 0, (void *)asm_proc_interrupt_234_handler, 1);
    proc_configure_idt_entry(235, 0, (void *)asm_proc_interrupt_235_handler, 1);
    proc_configure_idt_entry(236, 0, (void *)asm_proc_interrupt_236_handler, 1);
    proc_configure_idt_entry(237, 0, (void *)asm_proc_interrupt_237_handler, 1);
    proc_configure_idt_entry(238, 0, (void *)asm_proc_interrupt_238_handler, 1);
    proc_configure_idt_entry(239, 0, (void *)asm_proc_interrupt_239_handler, 1);
    proc_configure_idt_entry(240, 0, (void *)asm_proc_interrupt_240_handler, 1);
    proc_configure_idt_entry(241, 0, (void *)asm_proc_interrupt_241_handler, 1);
    proc_configure_idt_entry(242, 0, (void *)asm_proc_interrupt_242_handler, 1);
    proc_configure_idt_entry(243, 0, (void *)asm_proc_interrupt_243_handler, 1);
    proc_configure_idt_entry(244, 0, (void *)asm_proc_interrupt_244_handler, 1);
    proc_configure_idt_entry(245, 0, (void *)asm_proc_interrupt_245_handler, 1);
    proc_configure_idt_entry(246, 0, (void *)asm_proc_interrupt_246_handler, 1);
    proc_configure_idt_entry(247, 0, (void *)asm_proc_interrupt_247_handler, 1);
    proc_configure_idt_entry(248, 0, (void *)asm_proc_interrupt_248_handler, 1);
    proc_configure_idt_entry(249, 0, (void *)asm_proc_interrupt_249_handler, 1);
    proc_configure_idt_entry(250, 0, (void *)asm_proc_interrupt_250_handler, 1);
    proc_configure_idt_entry(251, 0, (void *)asm_proc_interrupt_251_handler, 1);
    proc_configure_idt_entry(252, 0, (void *)asm_proc_interrupt_252_handler, 1);
    proc_configure_idt_entry(253, 0, (void *)asm_proc_interrupt_253_handler, 1);
    proc_configure_idt_entry(254, 0, (void *)asm_proc_interrupt_254_handler, 1);
    proc_configure_idt_entry(255, 0, (void *)asm_proc_interrupt_255_handler, 1);
  }
}