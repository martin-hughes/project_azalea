/// @file
/// @brief x64-processor specific control functions.

#pragma once

#include "processor/processor.h"

/// @brief Processor information block - x64
///
/// Contains information the system will use to manage x64 processors.
struct processor_info_x64
{
  /// The ID of the local APIC for this processor. This allows the system to determine which processor it is running
  /// on, and is also used as the address when signalling other processors.
  uint32_t lapic_id;
};

typedef processor_info_generic<processor_info_x64> processor_info; ///< Processor info block on x64

extern processor_info *proc_info_block;
extern uint32_t processor_count;

/// @brief Indicies of known MSRS.
///
/// These correspond to the Intel documentation, so are not documented further.
enum class PROC_X64_MSRS : uint64_t
{
/// @cond
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

  IA32_FS_BASE = 0xC0000100,
  IA32_GS_BASE = 0xC0000101,
  IA32_KERNEL_GS_BASE = 0xC0000102,
/// @endcond
};

uint64_t proc_read_msr(PROC_X64_MSRS msr);
void proc_write_msr(PROC_X64_MSRS msr, uint64_t value);

/// @brief Execute the CPUID instruction on this CPU.
///
/// Parameter values can be found in the Intel documentation
///
/// @param eax_value The value of EAX when CPUID is executed
///
/// @param ecx_value The value of ECX when CPUID is executed
///
/// @param[out] ebx_eax The packed result stored in EBX and EAX
///
/// @param[out] edx_ecx The packed result stored in EDX and ECX
extern "C" void asm_proc_read_cpuid(uint64_t eax_value,
                                    uint64_t ecx_value,
                                    uint64_t *ebx_eax,
                                    uint64_t *edx_ecx);

uint64_t proc_x64_generate_msi_address(uint32_t kernel_proc_id);
