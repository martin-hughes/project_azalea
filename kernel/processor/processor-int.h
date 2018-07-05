#ifndef PROCESSOR_INTERNAL_H
#define PROCESSOR_INTERNAL_H

// Declarations internal to the processor/task-manager library.

#include <stdint.h>
#include "processor.h"

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
  uint64_t proc_rflags; /// RFLAGS. Set by processor during interrupt.
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
};
#pragma pack ( pop )

void *task_int_create_exec_context(ENTRY_PROC entry_point, task_thread *new_thread);
void task_int_delete_exec_context(task_thread *old_thread);

task_thread *task_get_next_thread();
extern "C" task_x64_exec_context *task_int_swap_task(uint64_t stack_addr, uint64_t cr3_value);

void task_install_task_switcher();
void task_platform_init();

extern "C" void proc_handle_irq(uint8_t irq_number);
void proc_irq_slowpath_thread();

task_process *task_create_system_process();

#endif
