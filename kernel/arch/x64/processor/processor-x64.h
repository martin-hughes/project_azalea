/// @file
/// @brief x64-processor specific control functions.

#pragma once

#include "processor.h"

/// @brief Processor information block - x64
///
/// Contains information the system will use to manage x64 processors.
struct processor_info_x64
{
  /// The ID of the local APIC for this processor. This allows the system to determine which processor it is running
  /// on, and is also used as the address when signalling other processors.
  uint32_t lapic_id;

  /// Starting addresses of the kernel stack
  void *kernel_stack_addr;

  /// Starting address of the stack used for interrupts handled by interrupt stack table entry 1.
  void *ist_1_addr;

  /// Starting address of the stack used for interrupts handled by interrupt stack table entry 2.
  void *ist_2_addr;

  /// Starting address of the stack used for interrupts handled by interrupt stack table entry 3.
  void *ist_3_addr;

  /// Starting address of the stack used for interrupts handled by interrupt stack table entry 4.
  void *ist_4_addr;
};

/// @brief Storage space for the stack of an x64 process.
///
/// This structure is only valid if the process is not currently running.
#pragma pack ( push, 1 )
struct task_x64_saved_stack
{
  // Parts relating to the task. These fields are saved by the kernel.
  uint8_t fx_state[512]; ///< Storage space used by the fxsave64/fxrstor64 instructions

  uint64_t r15; ///< Register R15
  uint64_t r14; ///< Register R14
  uint64_t r13; ///< Register R13
  uint64_t r12; ///< Register R12
  uint64_t r11; ///< Register R11
  uint64_t r10; ///< Register R10
  uint64_t r9;  ///< Register R9
  uint64_t r8;  ///< Register R8
  uint64_t rbp; ///< Register RBP
  uint64_t rdi; ///< Register RDI
  uint64_t rsi; ///< Register RSI
  uint64_t rdx; ///< Register RDX
  uint64_t rcx; ///< Register RCX
  uint64_t rbx; ///< Register RBX
  uint64_t rax; ///< Register RAX

  // These fields are pushed by the processor by it jumping to an interrupt.
  uint64_t proc_rip; ///< RIP to use when task restarts. Set by processor during interrupt.
  uint64_t proc_cs;  ///< CS of task. Set by processor during interrupt.
  uint64_t proc_rflags; ///< RFLAGS. Set by processor during interrupt.
  uint64_t proc_rsp; ///< RSP. Set by processor during interrupt.
  uint64_t proc_ss; ///< SS. Set by processor during interrupt.
};
#pragma pack ( pop )

static_assert(sizeof(task_x64_saved_stack) == 672, "Stack save structure size changed.");

/// @brief The execution context for a thread on x64.
///
/// It should never be necessary to access any of these fields outside of the x64 part of the task manager.
#pragma pack ( push , 1 )
struct task_x64_exec_context
{
  /// Page table pointer. Saved per-thread for simplicity, although it should be the same for all threads in a
  /// process. Note: This value is used referenced by offset in assembly language code.
  void *cr3_value;

  /// Stack pointer to use upon entry into system calls. Each thread needs its own stack, otherwise it is possible
  /// for concurrent system calls to overwrite each other's stacks.  Note: This value is used referenced by offset in
  /// assembly language code.
  void *syscall_stack;

  /// Space for saving the user mode process's stack while running a system call. Note: This value is used referenced
  /// by offset in assembly language code.
  void *user_mode_stack;

  /// The thread that this context belongs to. The address of this context is saved in the kernel GS, so this pointer
  /// can be used to retrieve the thread data.
  task_thread *owner_thread;

  /// The stack of the process. This field is filled in whenever the process is subject to an interrupt, so it is
  /// only valid if the process is not running.
  task_x64_saved_stack saved_stack;

  /// Value of FS Base for the process
  uint64_t fs_base;

  /// Value of GS Base for the process
  uint64_t gs_base;

  /// The original value of syscall_stack, to be used when the process exits to delete the stack (in case
  /// syscall_stack ever changes)
  void *orig_syscall_stack;
};
#pragma pack ( pop )

/// @cond
// Dunno why doxygen can't find the docs for these externs...
extern processor_info *proc_info_block;
extern processor_info_x64 *proc_info_x64_block;
extern uint32_t processor_count;
/// @endcond

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

extern "C" task_x64_exec_context *task_int_swap_task(uint64_t stack_addr, uint64_t cr3_value);
