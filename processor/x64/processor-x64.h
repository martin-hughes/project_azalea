#ifndef __PROCESSOR_X64_H
#define __PROCESSOR_X64_H

/// @brief Processor information block - x64
///
/// Contains information the system will use to manage x64 processors.
struct processor_info_x64
{
  /// The ID of the local APIC for this processor. This allows the system to determine which processor it is running
  /// on, and is also used as the address when signalling other processors.
  unsigned int lapic_id;
};

typedef processor_info_generic<processor_info_x64> processor_info;

enum class PROC_X64_MSRS
{
  IA32_APIC_BASE = 0x1b,
  IA32_MTRRCAP = 0xfe,
  IA32_MTRR_PHYSBASE0 = 0x200,
  IA32_MTRR_PHYSMASK0 = 0x201,
  IA32_MTRR_FIX64K_00000 = 0x250,
  IA32_MTRR_FIX16K_80000 = 0x258,
  IA32_MTRR_FIX16K_A0000 = 0x259,
  IA32_MTRR_FIX4K_C0000 = 0x268,
  IA32_MTRR_FIX4K_C8000 = 0x269,
  IA32_MTRR_FIX4K_D0000 = 0x26A,
  IA32_MTRR_FIX4K_D8000 = 0x26B,
  IA32_MTRR_FIX4K_E0000 = 0x26C,
  IA32_MTRR_FIX4K_E8000 = 0x26D,
  IA32_MTRR_FIX4K_F0000 = 0x26E,
  IA32_MTRR_FIX4K_F8000 = 0x26F,
  IA32_PAT = 0x277,
  IA32_MTRR_DEF_TYPE = 0x2FF,
};

unsigned long proc_read_msr(PROC_X64_MSRS msr);
void proc_write_msr(PROC_X64_MSRS msr, unsigned long value);

#endif
