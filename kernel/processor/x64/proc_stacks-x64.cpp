/// @file
/// @brief Allocate and deallocate stacks suitable for use in the x64 architecture.

//#define ENABLE_TRACING

#include "processor/processor.h"
#include "klib/klib.h"

namespace
{
  const uint64_t DEF_USER_MODE_STACK_PAGE = 0x000000000F000000;
}

/// @brief Allocate a single-page stack to the kernel.
///
/// @param kernel_mode True if a stack should be allocated for use within the kernel. False if a stack should be
///                    allocated for use within a user mode process.
///
/// @param proc If kernel_mode is true, this value *must* be nullptr. Otherwise it *must* point to a user-mode process
///             to allocate a stack in to.
///
/// @return An address that can be used as a stack pointer, growing downwards as far as the next page boundary. Values
///         are 16-byte aligned.
void *proc_allocate_stack(bool kernel_mode, task_process *proc)
{
  void *new_stack{nullptr};
  void *physical_backing{nullptr};

  KL_TRC_ENTRY;

  if (kernel_mode)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Kernel mode stack\n");
    ASSERT(proc == nullptr);

    // Allocate three pages but only map the middle one of them. This way if the stack over or underruns, a page fault
    // is generated.
    new_stack = mem_allocate_virtual_range(3);
    new_stack = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(new_stack) + MEM_PAGE_SIZE);

    physical_backing = mem_allocate_physical_pages(1);
    mem_map_range(physical_backing, new_stack, 1);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "User mode stack\n");
    ASSERT(proc != nullptr);
    ASSERT(!proc->kernel_mode);

    uint64_t stack_addr{DEF_USER_MODE_STACK_PAGE};
    const uint64_t double_page{MEM_PAGE_SIZE * 2};

    while (mem_get_phys_addr(reinterpret_cast<void *>(stack_addr), proc) != nullptr)
    {
      stack_addr -= double_page;
    }
    mem_vmm_allocate_specific_range(stack_addr, 1, proc);
    physical_backing = mem_allocate_physical_pages(1);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Physical backing page: ", physical_backing, "\n");
    mem_map_range(physical_backing, reinterpret_cast<void *>(stack_addr), 1, proc);

    new_stack = reinterpret_cast<void *>(stack_addr);
  }

  new_stack = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(new_stack) + MEM_PAGE_SIZE - 16);
  ASSERT((reinterpret_cast<uint64_t>(new_stack) & 0x0F) == 0);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", new_stack, "\n");
  KL_TRC_EXIT;

  return new_stack;
}

/// @brief Deallocate a previously allocated single page stack.
///
/// This function will only work for stacks in kernel space. Deallocating a user-mode stack should be left to that
/// process to complete itself.
///
/// @param stack_ptr Pointer to any place in the stack to deallocate.
void proc_deallocate_stack(void *stack_ptr)
{
  KL_TRC_ENTRY;

  uint64_t stack_page_addr = reinterpret_cast<uint64_t>(stack_ptr);
  ASSERT(stack_page_addr > 0x8000000000000000);
  stack_page_addr -= (stack_page_addr % MEM_PAGE_SIZE);
  // stack_page_addr now equals the beginning of the mapped page. Release the physical page as well.
  mem_unmap_range(reinterpret_cast<void *>(stack_page_addr), 1, nullptr, true);

  stack_page_addr -= MEM_PAGE_SIZE;
  // stack_page_addr now points at the first of the three virtual pages assigned to this stack.
  mem_deallocate_virtual_range(reinterpret_cast<void *>(stack_page_addr), 3);

  KL_TRC_EXIT;
}
