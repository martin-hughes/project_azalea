/// @file virtual.cpp
/// @brief Kernel core memory manager - Virtual memory manager.
///
/// The virtual memory manager is responsible for allocating virtual memory ranges to the caller. The caller is
/// responsible for backing these ranges with physical memory pages.
///
/// Virtual address space info is stored in a linked list. Each element of the list stores details of a range - whether
/// it is allocated, and its length. Each "lump" is a power-of-two number of pages.
///
/// When a new request is made, the allocated is rounded to the next largest power-of-two number of pages. The list is
/// searched for the smallest deallocated lump that will fit the request. If it is too big, it should be the next
/// power-of-two or more larger, and it is divided in two repeatedly until the correct sized lump exists and can be
/// returned. Details of it and the remaining (now smaller) lumps are added to the information list and the original
/// lump removed.
///
/// When a lump is deallocated, its neighbours in the list are considered to see whether they will form a larger
/// power-of-two sized block. If it can, the two neighbour-lumps are coalesced and replaced in the range information
/// list by one entry.
///
/// In some ways this represents an easy-to-implement buddy allocation system.
///
/// Only one thread may access the virtual allocation system at once.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "mem/mem.h"
#include "mem/mem-int.h"
#include "processor/processor.h"

/// Whether or not the Virtual Memory Manager is initialised.
static bool vmm_initialized = false;

namespace
{
  // Store the kernel's process data in a global object.
  vmm_process_data kernel_vmm_data;

  // Use these arrays for the initial startup of the memory manager. If there isn't a predefined space we get in to a
  // chicken-and-egg state - how does the memory manager allocate memory for itself?
  const unsigned int NUM_INITIAL_RANGES = 64;
  klib_list_item<vmm_range_data *> initial_range_list[NUM_INITIAL_RANGES];
  vmm_range_data initial_range_data[NUM_INITIAL_RANGES];
  unsigned int initial_ranges_used;
  unsigned int initial_list_items_used;

  // Support function declarations
  void mem_vmm_initialize();
  klib_list_item<vmm_range_data *> *mem_vmm_split_range(klib_list_item<vmm_range_data *> *item_to_split,
                                                        unsigned int number_of_pages_reqd,
                                                        vmm_process_data *proc_data_ptr);
  klib_list_item<vmm_range_data *> *mem_vmm_get_suitable_range(unsigned int num_pages,
                                                               vmm_process_data *proc_data_ptr);
  void mem_vmm_resolve_merges(klib_list_item<vmm_range_data *> *start_point);

  klib_list_item<vmm_range_data *> *mem_vmm_allocate_list_item(vmm_process_data *proc_data_ptr);
  vmm_range_data *mem_vmm_allocate_range_item(vmm_process_data *proc_data_ptr);
  void mem_vmm_free_list_item (klib_list_item<vmm_range_data *> *item);
  void mem_vmm_free_range_item(vmm_range_data *item);
  bool mem_vmm_lock(vmm_process_data *proc_data_ptr);
  void mem_vmm_unlock(vmm_process_data *proc_data_ptr);
};

//------------------------------------------------------------------------------
// Memory manager main interface functions.
//------------------------------------------------------------------------------

/// @brief Allocate some room in the kernel's virtual memory space.
///
/// Allocate the specified number of pages in the kernel's virtual memory space and return the address of that
/// allocation This function does not attempt to create a mapping to physical memory, nor will it allow the allocation
/// of memory within user space.
///
/// Independent of the requested number of pages to allocate, the actual allocation will be either 1, or an integer
/// power of two number of pages. This may cause any stats about RAM allocations to differ from the user's expectations
/// but they should not rely on the extra pages being available.
///
/// Users must use #mem_map_range to map the result to physical memory before use, or a page fault will result.
///
/// @param num_pages The number of pages to allocate. Need not be a power of two.
///
/// @param process_to_use The process to do the allocation in. If nullptr, a kernel allocation is made.
///
/// @return The address of the virtual range allocated.
///
/// @see mem_deallocate_virtual_range
void *mem_allocate_virtual_range(unsigned int num_pages, task_process *process_to_use)
{
  vmm_process_data *proc_data_ptr;
  klib_list_item<vmm_range_data *> *selected_list_item;
  vmm_range_data *selected_range_data;
  bool acquired_lock;

  KL_TRC_ENTRY;

  if (!vmm_initialized)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Initialising memory manager.\n");
    mem_vmm_initialize();
  }

  if (process_to_use == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Using kernel data\n");
    proc_data_ptr = &kernel_vmm_data;
  }
  else
  {
    ASSERT(process_to_use->mem_info != nullptr);
    proc_data_ptr = &process_to_use->mem_info->process_vmm_data;
  }


  acquired_lock = mem_vmm_lock(proc_data_ptr);
  KL_TRC_DATA("Lock acquired?", acquired_lock);

  // How many pages are we actually going to allocate. If num_pages is exactly
  // a power of two, it will be used. Otherwise, it will be rounded up to the
  // next power of two.
  unsigned int actual_num_pages;
  actual_num_pages = round_to_power_two(num_pages);

  // What range are we going to allocate from?
  selected_list_item = mem_vmm_get_suitable_range(actual_num_pages, proc_data_ptr);
  selected_range_data = (vmm_range_data *)selected_list_item->item;

  // If this range is too large, split it in to pieces. Otherwise, simply mark
  // it allocated and return it.
  ASSERT(selected_range_data->number_of_pages >= actual_num_pages);
  ASSERT(selected_range_data->allocated == false);

  if (selected_range_data->number_of_pages != actual_num_pages)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Splitting over-sized page.\n");
    selected_list_item = mem_vmm_split_range(selected_list_item, actual_num_pages, proc_data_ptr);
    selected_range_data = (vmm_range_data *)selected_list_item->item;

  }
  ASSERT(selected_range_data->number_of_pages == actual_num_pages);
  selected_range_data->allocated = true;

  if (acquired_lock)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Releasing lock\n");
    mem_vmm_unlock(proc_data_ptr);
  }

  KL_TRC_EXIT;

  return (void *)selected_range_data->start;
}

/// @brief Allocate a specific range of virtual memory.
///
/// Allocate a specific range of virtual memory. This is primarily used when setting up VMM, in order that it knows
/// about the memory already in use for the kernel.
///
/// @param start_addr The beginning of the range to allocate. This must be an address that's a multiple of num_pages
///                   and a multiple of MEM_PAGE_SIZE.
///
/// @param num_pages The number of pages covered by this allocation. Must be an integer (or zero) power of two.
///
/// @param process_to_use The data for the process to perform the allocation in.
void mem_vmm_allocate_specific_range(unsigned long start_addr, unsigned int num_pages, task_process *process_to_use)
{
  vmm_process_data *proc_data_ptr;
  klib_list_item<vmm_range_data *> *cur_item;
  vmm_range_data *cur_data;
  unsigned long end_addr;
  unsigned int rounded_num_pages = round_to_power_two(num_pages);

  KL_TRC_ENTRY;

  if (process_to_use == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Using kernel data\n");
    proc_data_ptr = &kernel_vmm_data;
  }
  else
  {
    ASSERT(process_to_use->mem_info != nullptr);
    proc_data_ptr = &process_to_use->mem_info->process_vmm_data;
  }

  // Check that start_addr is on a boundary that matches the number of pages
  // requested.
  ASSERT(rounded_num_pages == num_pages);
  ASSERT((start_addr % (num_pages * MEM_PAGE_SIZE)) == 0);

  // Look for the range that contains this memory address. Split it down to
  // size.
  cur_item = proc_data_ptr->vmm_range_data_list.head;
  while (cur_item != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "cur_item != NULL\n");
    cur_data = (vmm_range_data *)cur_item->item;

    // Have end_addr be one lower than it seems like it ought to be. This
    // prevents any issues with ranges that end exactly at the top of RAM, and
    // will never interfere with the calculation (since pages will always be
    // larger than 1 byte in size!)
    end_addr = cur_data->start + (cur_data->number_of_pages * MEM_PAGE_SIZE) -1;
    if ((cur_data->start <= start_addr) && (end_addr > start_addr))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Correct range found\n");
      ASSERT(cur_data->number_of_pages >= num_pages);

      // If the range we've found is the correct size - perfect. Allocate it and
      // carry on. Otherwise it must be too large. Split it in two and try again
      // to allocate it.
      if (cur_data->number_of_pages == num_pages)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Correct size found\n");
        ASSERT(!cur_data->allocated);
        cur_data->allocated = true;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Size too large\n");
        cur_item = mem_vmm_split_range(cur_item, cur_data->number_of_pages / 2, proc_data_ptr);

        mem_vmm_allocate_specific_range(start_addr, num_pages, process_to_use);
      }

      KL_TRC_EXIT;
      return;
    }

    cur_item = cur_item->next;
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Item found\n");

  // Presumably this means we tried to get a range that's not owned by the
  // kernel.
  ASSERT(cur_item != nullptr);

  KL_TRC_EXIT;
}

/// @brief Deallocate pages allocated earlier in the kernels virtual memory space.
///
/// Deallocate a range of virtual memory that was allocated by #mem_allocate_virtual_range.
///
/// The user is responsible for cleaning up the physical memory that was backing this range.
///
/// @param start The address of the virtual range. This must be exactly as returned from #mem_allocate_virtual_range or
///              the allocation will fail - or worse, might scribble other allocations.
///
/// @param num_pages The number of pages allocated by #mem_allocate_virtual_range. This must be the same as the value
//                   passed to #mem_allocate_virtual_range, or an error will result.
///
/// @param process_to_use The process to do the deallocation in. If nullptr, a kernel deallocation is made.
void mem_deallocate_virtual_range(void *start, unsigned int num_pages, task_process *process_to_use)
{
  vmm_process_data *proc_data_ptr;

  KL_TRC_ENTRY;

  if (process_to_use == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Using kernel data\n");
    proc_data_ptr = &kernel_vmm_data;
  }
  else
  {
    ASSERT(process_to_use->mem_info != nullptr);
    proc_data_ptr = &process_to_use->mem_info->process_vmm_data;
  }

  klib_list_item<vmm_range_data *> *cur_list_item;
  vmm_range_data *cur_range_data;
  unsigned int actual_num_pages;
  bool acquired_lock;

  ASSERT(vmm_initialized);

  acquired_lock = mem_vmm_lock(proc_data_ptr);
  KL_TRC_DATA("Lock acquired?", acquired_lock);

  actual_num_pages = round_to_power_two(num_pages);

  cur_list_item = proc_data_ptr->vmm_range_data_list.head;
  while (cur_list_item != nullptr)
  {
    cur_range_data = (vmm_range_data *)cur_list_item->item;
    if (cur_range_data->start == (unsigned long)start)
    {
      ASSERT(cur_range_data->allocated == true);
      ASSERT(cur_range_data->number_of_pages == actual_num_pages);
      cur_range_data->allocated = false;

      mem_vmm_resolve_merges(cur_list_item);

      if (acquired_lock)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Releasing lock\n");
        mem_vmm_unlock(proc_data_ptr);
      }

      KL_TRC_EXIT;
      return;
    }

    cur_list_item = cur_list_item->next;
  }

  // If we reach this point, we haven't been able to deallocate this range. It
  // probably wasn't a valid range to start with. Bail out.
  ASSERT(cur_list_item != nullptr);

  if (acquired_lock)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Releasing lock\n");
    mem_vmm_unlock(proc_data_ptr);
  }

  KL_TRC_EXIT;
}

/// Initialize the virtual-memory manager specific process data structure.
///
/// @param proc_data_ptr The process data structure to initialize.
void mem_vmm_init_proc_data(vmm_process_data &proc_data_ref)
{
  klib_list_item<vmm_range_data *> *root_item;
  vmm_range_data *root_data;
  KL_TRC_ENTRY;

  proc_data_ref.vmm_lock = 0;
  klib_list_initialize(&proc_data_ref.vmm_range_data_list);
  proc_data_ref.vmm_user_thread_id = nullptr;

  root_item = mem_vmm_allocate_list_item(&proc_data_ref);
  root_data = mem_vmm_allocate_range_item(&proc_data_ref);
  klib_list_item_initialize(root_item);
  root_item->item = root_data;
  klib_list_add_head(&proc_data_ref.vmm_range_data_list, root_item);
  root_data->allocated = false;
  root_data->start = 0x0000000000000000;

  // This should be the maximum number of 2MB pages when using 48-bit virtual memory addresses and half the space is
  // reserved for the kernel.
  root_data->number_of_pages = 0x2000000;

  KL_TRC_EXIT;
}

//------------------------------------------------------------------------------
// Support functions.
//------------------------------------------------------------------------------

namespace
{
  /// @brief Sets up the Virtual Memory Manager.
  ///
  /// Must only be called once.
  void mem_vmm_initialize()
  {
    KL_TRC_ENTRY;

    klib_list_item<vmm_range_data *> *root_item;
    vmm_range_data *root_data;
    unsigned long free_pages;
    unsigned long used_pages;

    ASSERT(!vmm_initialized);

    initial_ranges_used = 0;
    initial_list_items_used = 0;
    klib_list_initialize(&kernel_vmm_data.vmm_range_data_list);

    // Set up a range item to cover the entirety of the kernel's available virtual
    // memory space.
    root_item = mem_vmm_allocate_list_item(&kernel_vmm_data);
    root_data = mem_vmm_allocate_range_item(&kernel_vmm_data);
    klib_list_item_initialize(root_item);
    root_item->item = root_data;
    klib_list_add_head(&kernel_vmm_data.vmm_range_data_list, root_item);
    root_data->allocated = false;
    root_data->start = 0xFFFFFFFF00000000;
    root_data->number_of_pages = 2048;

    // Allocate the ranges we already know are in use. These are:
    // - The kernel's image. 0xFFFFFFFF00000000 - (+2MB)
    //     N.B. The kernel actually starts at 1MB higher than this, and is
    //     currently limited to 1MB in size.
    // - Page table modification area: 0xFFFFFFFFFFFE0000 - end.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Allocating first range.\n");
    mem_vmm_allocate_specific_range(0xFFFFFFFF00000000, 1, nullptr);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Allocating second range.\n");
    mem_vmm_allocate_specific_range(0xFFFFFFFFFFE00000, 1, nullptr);

    free_pages = 0;
    used_pages = 0;

    root_item = kernel_vmm_data.vmm_range_data_list.head;
    ASSERT(root_item != nullptr);
    while (root_item != nullptr)
    {
      root_data = (vmm_range_data *)root_item->item;
      ASSERT(root_data != nullptr);
      if (root_data->allocated)
      {
        used_pages += root_data->number_of_pages;
      }
      else
      {
        free_pages += root_data->number_of_pages;
      }
      root_item = root_item->next;
    }

    klib_synch_spinlock_init(kernel_vmm_data.vmm_lock);
    kernel_vmm_data.vmm_user_thread_id = 0;

    ASSERT(free_pages > 5);
    ASSERT(used_pages < 20);

    vmm_initialized = true;

    KL_TRC_EXIT;
  }

  /// @brief Return the smallest range still available that is still larger than or equal to num_pages.
  ///
  /// @param num_pages The minimum number of pages required in the range.
  ///
  /// @param proc_data_ptr The data for the process to perform the allocation in.
  klib_list_item<vmm_range_data *> *mem_vmm_get_suitable_range(unsigned int num_pages, vmm_process_data *proc_data_ptr)
  {
    KL_TRC_ENTRY;

    ASSERT(proc_data_ptr != nullptr);

    klib_list_item<vmm_range_data *> *cur_range_item;
    klib_list_item<vmm_range_data *> *selected_range_item = nullptr;
    vmm_range_data *cur_range;
    vmm_range_data *selected_range = nullptr;

    ASSERT(!klib_list_is_empty(&proc_data_ptr->vmm_range_data_list));
    ASSERT(num_pages != 0);
    ASSERT(vmm_initialized);

    // Spin through the list of range data to look for the smallest suitable
    // range.
    cur_range_item = proc_data_ptr->vmm_range_data_list.head;
    while(cur_range_item != nullptr)
    {
      //KL_TRC_TRACE((TRC_LVL_FLOW, "Looking at next item.\n"));
      cur_range = (vmm_range_data *)cur_range_item->item;
      ASSERT(cur_range != nullptr);
      if ((cur_range->allocated == false) &&
          (cur_range->number_of_pages >= num_pages) &&
          ((selected_range == nullptr) ||
          (selected_range->number_of_pages > cur_range->number_of_pages)))
      {
        //KL_TRC_TRACE((TRC_LVL_FLOW, "Item found.\n"));
        selected_range = cur_range;
        selected_range_item = cur_range_item;
      }
      cur_range_item = cur_range_item->next;
    }

    KL_TRC_EXIT;

    ASSERT(selected_range_item != nullptr);
    return selected_range_item;
  }

  /// @brief Split a range that is unnecessarily large into smaller ranges.
  ///
  /// Split a large memory range in to two or more smaller chunks, so as to be left with at least one that's exactly
  /// number_of_pages_reqd in length.
  ///
  /// Each range is required to be a power of two number of pages in size, so this function operates by dividing a
  /// range into two, and then recursively calling itself on one of the two new ranges until it has a suitably sized
  /// range to return.
  ///
  /// @param item_to_split The range which is too large and needs splitting.
  ///
  /// @param number_of_pages_reqd The minimum number of pages that must be contained in the range returned.
  ///
  /// @param proc_data_ptr The data for the process to perform the allocation in.
  ///
  /// @return A range item of the correct size (or larger) The caller need not clean this up, it lives in the list of
  ///         ranges.
  klib_list_item<vmm_range_data *> *mem_vmm_split_range(klib_list_item<vmm_range_data *> *item_to_split,
                                                        unsigned int number_of_pages_reqd,
                                                        vmm_process_data *proc_data_ptr)
  {
    KL_TRC_ENTRY;

    ASSERT(proc_data_ptr != nullptr);

    klib_list_item<vmm_range_data *> *second_half_of_split;
    vmm_range_data *old_range_data;
    vmm_range_data *new_range_data;

    // Allocate a new list item and range data. Use these special functions since
    // VMM manages its own memory.
    second_half_of_split = mem_vmm_allocate_list_item(proc_data_ptr);
    new_range_data = mem_vmm_allocate_range_item(proc_data_ptr);

    // Add the new range to the list of ranges after the old one. We'll always
    // pass back the first half of the pair.
    second_half_of_split->item = new_range_data;
    klib_list_add_after(item_to_split, second_half_of_split);

    old_range_data = (vmm_range_data *)item_to_split->item;
    old_range_data->number_of_pages = old_range_data->number_of_pages / 2;
    new_range_data->number_of_pages = old_range_data->number_of_pages;
    new_range_data->allocated = false;
    new_range_data->start = old_range_data->start +
                                (new_range_data->number_of_pages * MEM_PAGE_SIZE);

    // If the pages are still too large, split the first half down again. Don't do
    // the second half - it's far more useful left as a large range.
    if (new_range_data->number_of_pages > number_of_pages_reqd)
    {
      item_to_split = mem_vmm_split_range(item_to_split, number_of_pages_reqd, proc_data_ptr);
    }

    KL_TRC_EXIT;

    return item_to_split;
  }

  /// @brief See whether a recently freed range can be merged with its partner and merge if so.
  ///
  /// Once a range has been released, see if its neighbour is free. If it is, these ranges can be combined to form a
  /// larger range - which is useful, since allocations can be smaller than an available range, but not larger. If the
  /// two ranges have merged, its possible that the newly merged range can merge with its neighbour, so recurse until
  /// no more merges can occur.
  ///
  /// @param start_point A newly freed range.
  void mem_vmm_resolve_merges(klib_list_item<vmm_range_data *> *start_point)
  {
    KL_TRC_ENTRY;

    vmm_range_data *this_data;
    klib_list_item<vmm_range_data *> *partner_item;
    vmm_range_data *partner_data;
    bool first_half_of_pair;
    unsigned long next_block_size;

    klib_list_item<vmm_range_data *> *survivor_item;
    vmm_range_data *survivor_data;
    klib_list_item<vmm_range_data *> *released_item;
    vmm_range_data *released_data;

    ASSERT (start_point != nullptr);

    // We want to merge in the reverse way that we split items. This means that
    // the address of the newly merged block must be a multiple of the size of
    // that block.
    this_data = (vmm_range_data *)start_point->item;
    ASSERT(this_data->allocated == false);
    next_block_size = this_data->number_of_pages * 2;
    first_half_of_pair = ((this_data->start % next_block_size) == 0);

    // Based on the address and range size, select which range it may be possible
    // to merge with.
    if (first_half_of_pair)
    {
      partner_item = start_point->next;
    }
    else
    {
      partner_item = start_point->prev;
    }
    partner_data = (vmm_range_data *)partner_item->item;

    if ((!partner_data->allocated) &&
        (partner_data->number_of_pages == this_data->number_of_pages))
    {
      // Since both this range and its partner are deallocated and the same size
      // they can be merged. This means that one of the ranges can be freed.
      if (first_half_of_pair)
      {
        survivor_item = start_point;
        survivor_data = this_data;
        released_item = partner_item;
        released_data = partner_data;
      }
      else
      {
        released_item = start_point;
        released_data = this_data;
        survivor_item = partner_item;
        survivor_data = partner_data;
      }

      // Make the survivor twice as large and free the range that's no longer
      // relevant.
      survivor_data->number_of_pages = survivor_data->number_of_pages * 2;
      klib_list_remove(released_item);
      mem_vmm_free_list_item(released_item);
      mem_vmm_free_range_item(released_data);

      // Since we've merged at this level, it's possible that the newly-enlarged
      // range can be merged with the its partner too.
      mem_vmm_resolve_merges(survivor_item);
    }

    KL_TRC_EXIT;
  }

  //------------------------------------------------------------------------------
  // Internal memory management code.
  //------------------------------------------------------------------------------

  /// @brief Allocate a list item for use in the range management code.
  ///
  /// Allocate a new list item. In order that it is possible to allocate list items before the memory manager is fully
  /// initialised, there is small list of items to be used before the MM is ready.
  ///
  /// Note the similarity with #mem_vmm_allocate_range_item.
  ///
  /// @return An allocated list item. This muse be passed to #mem_vmm_free_list_item to destroy it.
  ///
  /// @param proc_data_ptr The data for the process to perform the allocation in.
  klib_list_item<vmm_range_data *> *mem_vmm_allocate_list_item(vmm_process_data *proc_data_ptr)
  {
    KL_TRC_ENTRY;

    klib_list_item<vmm_range_data *> *ret_item;

    // Use one of the preallocated "initial_range_list" items if any are left.
    // There should be enough to last until VMM is fully initialised, at which
    // point grabbing them from kmalloc should be fine.
    if ((proc_data_ptr != &kernel_vmm_data) || (initial_list_items_used >= NUM_INITIAL_RANGES))
    {
      ASSERT(vmm_initialized);
      ret_item = reinterpret_cast<klib_list_item<vmm_range_data *> *>(kmalloc(sizeof(klib_list_item<vmm_range_data *>)));
    }
    else
    {
      ret_item = &initial_range_list[initial_list_items_used];
      initial_list_items_used++;
    }

    klib_list_item_initialize(ret_item);

    KL_TRC_EXIT;

    return ret_item;
  }

  /// @brief Allocate a range item for use in the range management code.
  ///
  /// Allocate a new range item. In order that it is possible to allocate range items before the memory manager is
  /// fully initialised, there is small list of items to be used before the MM is ready.
  ///
  /// Note the similarity with #mem_vmm_allocate_list_item.
  ///
  /// @param proc_data_ptr The data for the process to perform the allocation in.
  ///
  /// @return An allocated list item. This muse be passed to #mem_vmm_free_range_item to destroy it.
  vmm_range_data *mem_vmm_allocate_range_item(vmm_process_data *proc_data_ptr)
  {
    KL_TRC_ENTRY;

    vmm_range_data *ret_item;

    // Use one of the preallocated "initial_range_list" items if any are left.
    // There should be enough to last until VMM is fully initialised, at which
    // point grabbing them from kmalloc should be fine.
    if ((proc_data_ptr != &kernel_vmm_data) || (initial_ranges_used >= NUM_INITIAL_RANGES))
    {
      ASSERT(vmm_initialized);
      ret_item = (vmm_range_data *)kmalloc(sizeof(vmm_range_data));
    }
    else
    {
      ret_item = &initial_range_data[initial_ranges_used];
      initial_ranges_used++;
    }

    KL_TRC_EXIT;

    return ret_item;
  }

  /// @brief Free a list item allocated by #mem_vmm_allocate_list_item
  ///
  /// Free a list item allocated by #mem_vmm_allocate_list_item. This takes care of returning the relevant items to the
  /// list of allocations that is used before the VMM is fully allocated, and returns the rest to #kfree
  ///
  /// Note the similarity with #mem_vmm_free_range_item.
  ///
  /// @param item The item to free
  void mem_vmm_free_list_item (klib_list_item<vmm_range_data *> *item)
  {
    KL_TRC_ENTRY;

    // If this item is one of the pre-allocated ones, there's nothing to do.
    // Otherwise, hand it over to kfree.
    unsigned long this_item = (unsigned long)item;
    unsigned long prealloc_start = (unsigned long)(&initial_range_list[0]);
    unsigned long prealloc_end = prealloc_start + sizeof(initial_range_list);

    if ((this_item < prealloc_start) || (this_item >= prealloc_end))
    {
      kfree(item);
    }

    KL_TRC_EXIT;
  }

  /// @brief Free a list item allocated by #mem_vmm_allocate_range_item
  ///
  /// Free a list item allocated by #mem_vmm_allocate_range_item. This takes care of returning the relevant items to
  /// the list of allocations that is used before the VMM is fully allocated, and returns the rest to #kfree
  ///
  /// Note the similarity with #mem_vmm_free_list_item.
  ///
  /// @param item The item to free
  void mem_vmm_free_range_item (vmm_range_data *item)
  {
    KL_TRC_ENTRY;

    // If this item is one of the pre-allocated ones, there's nothing to do.
    // Otherwise, hand it over to kfree.
    unsigned long this_item = (unsigned long)item;
    unsigned long prealloc_start = (unsigned long)(&initial_range_data[0]);
    unsigned long prealloc_end = prealloc_start + sizeof(initial_range_data);

    if ((this_item < prealloc_start) || (this_item >= prealloc_end))
    {
      kfree(item);
    }

    KL_TRC_EXIT;
  }

  /// @brief Ensure that the current thread is the only user of the VMM.
  ///
  /// @param proc_data_ptr The process the lock is being acquired for.
  ///
  /// @return TRUE if the lock had to be acquired, FALSE if this thread already had the lock.
  bool mem_vmm_lock(vmm_process_data *proc_data_ptr)
  {
    KL_TRC_ENTRY;

    ASSERT(proc_data_ptr != nullptr);

    if (proc_data_ptr->vmm_user_thread_id != task_get_cur_thread())
    {
      klib_synch_spinlock_lock(proc_data_ptr->vmm_lock);
      proc_data_ptr->vmm_user_thread_id = task_get_cur_thread();
      return true;
    }

    return false;

    KL_TRC_EXIT;
  }

  /// @brief This thread has finished using VMM, so allow other threads to instead.
  ///
  /// @param proc_data_ptr The process the lock was acquired in.
  void mem_vmm_unlock(vmm_process_data *proc_data_ptr)
  {
    ASSERT(proc_data_ptr != nullptr);

    proc_data_ptr->vmm_user_thread_id = 0;
    klib_synch_spinlock_unlock(proc_data_ptr->vmm_lock);
  }
};