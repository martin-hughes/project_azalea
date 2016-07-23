// Kernel core memory manager - Virtual memory manager.
//
// The virtual memory manager is responsible for allocating virtual memory
// ranges to the caller. The caller is responsible for backing these ranges
// with physical memory pages.
//
// Virtual address space info is stored in a linked list. Each element of the
// list stores details of a range - whether it is allocated, and its length.
// Each "lump" is a power-of-two number of pages.
//
// When a new request is made, the allocated is rounded to the next largest
// power-of-two number of pages. The list is searched for the smallest
// deallocated lump that will fit the request. If it is too big, it should be
// the next power-of-two or more larger, and it is divided in two repeatedly
// until the correct sized lump exists and can be returned. Details of it and
// the remaining (now smaller) lumps are added to the information list and the
// original lump removed.
//
// When a lump is deallocated, its neighbours in the list are considered to see
// whether they will form a larger power-of-two sized block. If it can, the two
// neighbour-lumps are coalesced and replaced in the range information list by
// one entry.
//
// In some ways this represents an easy-to-implement buddy allocation system.

//#define ENABLE_TRACING

#include "mem/mem.h"
#include "mem/mem_internal.h"
#include "klib/klib.h"

bool vmm_initialized = false;
klib_list vmm_range_data_list;

struct vmm_range_data
{
  unsigned long start;
  unsigned long number_of_pages;
  bool allocated;
};

// Use these arrays for the initial startup of the memory manager. If there
// isn't a predefined space we get in to a chicken-and-egg state - how does the
// memory manager allocate memory for itself?

const unsigned int NUM_INITIAL_RANGES = 64;
klib_list_item initial_range_list[NUM_INITIAL_RANGES];
vmm_range_data initial_range_data[NUM_INITIAL_RANGES];
unsigned int initial_ranges_used;
unsigned int initial_list_items_used;

// Support function declarations
void mem_vmm_initialize();
klib_list_item *mem_vmm_split_range(klib_list_item *item_to_split,
                                    unsigned int number_of_pages_reqd);
klib_list_item *mem_vmm_get_suitable_range(unsigned int num_pages);
void mem_vmm_resolve_merges(klib_list_item *start_point);
void mem_vmm_allocate_specific_range(unsigned long start_addr,
                                     unsigned int num_pages);

klib_list_item *mem_vmm_allocate_list_item();
vmm_range_data *mem_vmm_allocate_range_item();
void mem_vmm_free_list_item (klib_list_item *item);
void mem_vmm_free_range_item(vmm_range_data *item);

//------------------------------------------------------------------------------
// Memory manager main interface functions.
//------------------------------------------------------------------------------

// Allocate some room in the kernel's virtual memory space.
void *mem_allocate_virtual_range(unsigned int num_pages)
{
  KL_TRC_ENTRY;

  klib_list_item *selected_list_item;
  vmm_range_data *selected_range_data;

  if (!vmm_initialized)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Initializing memory manager.\n"));
    mem_vmm_initialize();
  }

  // How many pages are we actually going to allocate. If num_pages is exactly
  // a power of two, it will be used. Otherwise, it will be rounded up to the
  // next power of two.
  unsigned int actual_num_pages;
  actual_num_pages = round_to_power_two(num_pages);

  // What range are we going to allocate from?
  selected_list_item = mem_vmm_get_suitable_range(actual_num_pages);
  selected_range_data = (vmm_range_data *)selected_list_item->item;

  // If this range is too large, split it in to pieces. Otherwise, simply mark
  // it allocated and return it.
  ASSERT(selected_range_data->number_of_pages >= actual_num_pages);
  ASSERT(selected_range_data->allocated == false);

  if (selected_range_data->number_of_pages != actual_num_pages)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Splitting over-sized page.\n"));
    selected_list_item = mem_vmm_split_range(selected_list_item,
                                             actual_num_pages);
    selected_range_data = (vmm_range_data *)selected_list_item->item;

  }
  ASSERT(selected_range_data->number_of_pages == actual_num_pages);
  selected_range_data->allocated = true;

  KL_TRC_EXIT;

  return (void *)selected_range_data->start;
}

// Deallocate pages allocated earlier in the kernels virtual memory space.
void mem_deallocate_virtual_range(void *start, unsigned int num_pages)
{
  KL_TRC_ENTRY;

  klib_list_item *cur_list_item;
  vmm_range_data *cur_range_data;
  unsigned int actual_num_pages;

  ASSERT(vmm_initialized);

  actual_num_pages = round_to_power_two(num_pages);

  cur_list_item = vmm_range_data_list.head;
  while (cur_list_item != NULL)
  {
    cur_range_data = (vmm_range_data *)cur_list_item->item;
    if (cur_range_data->start == (unsigned long)start)
    {
      ASSERT(cur_range_data->allocated == true);
      ASSERT(cur_range_data->number_of_pages == actual_num_pages);
      cur_range_data->allocated = false;

      mem_vmm_resolve_merges(cur_list_item);

      KL_TRC_EXIT;
      return;
    }

    cur_list_item = cur_list_item->next;
  }

  // If we reach this point, we haven't been able to deallocate this range. It
  // probably wasn't a valid range to start with. Bail out.
  ASSERT(cur_list_item != NULL);

  KL_TRC_EXIT;
}

// Straightforward set up of the virtual memory manager system.
void mem_vmm_initialize()
{
  KL_TRC_ENTRY;

  klib_list_item *root_item;
  vmm_range_data *root_data;
  unsigned long free_pages;
  unsigned long used_pages;

  ASSERT(!vmm_initialized);

  initial_ranges_used = 0;
  initial_list_items_used = 0;
  klib_list_initialize(&vmm_range_data_list);

  // Set up a range item to cover the entirety of the kernel's available virtual
  // memory space. TODO: Some of this is x64 specific. Move it there.
  root_item = mem_vmm_allocate_list_item();
  root_data = mem_vmm_allocate_range_item();
  klib_list_item_initialize(root_item);
  root_item->item = root_data;
  klib_list_add_head(&vmm_range_data_list, root_item);
  root_data->allocated = false;
  root_data->start = 0xFFFFFFFF00000000;
  root_data->number_of_pages = 2048;

  // Allocate the ranges we already know are in use. These are:
  // - The kernel's image. 0xFFFFFFFF00000000 - (+2MB)
  //     N.B. The kernel actually starts at 1MB higher than this, and is
  //     currently limited to 1MB in size.
  // - Page table modification area: 0xFFFFFFFFFFFE0000 - end.
  KL_TRC_TRACE((TRC_LVL_FLOW, "Allocating first range.\n"));
  mem_vmm_allocate_specific_range(0xFFFFFFFF00000000, 1);
  KL_TRC_TRACE((TRC_LVL_FLOW, "Allocating second range.\n"));
  mem_vmm_allocate_specific_range(0xFFFFFFFFFFE00000, 1);

  free_pages = 0;
  used_pages = 0;

  root_item = vmm_range_data_list.head;
  ASSERT(root_item != NULL);
  while (root_item != NULL)
  {
    root_data = (vmm_range_data *)root_item->item;
    ASSERT(root_data != NULL);
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

  ASSERT(free_pages > 5);
  ASSERT(used_pages < 20);

  vmm_initialized = true;

  KL_TRC_EXIT;
}

//------------------------------------------------------------------------------
// Support functions.
//------------------------------------------------------------------------------

// Return the smallest range still available that is still larger than or equal
// to num_pages.
klib_list_item *mem_vmm_get_suitable_range(unsigned int num_pages)
{
  KL_TRC_ENTRY;

  klib_list_item *cur_range_item;
  klib_list_item *selected_range_item = (klib_list_item *)NULL;
  vmm_range_data *cur_range;
  vmm_range_data *selected_range = (vmm_range_data *)NULL;

  ASSERT(!klib_list_is_empty(&vmm_range_data_list));
  ASSERT(num_pages != 0);
  ASSERT(vmm_initialized);

  // Spin through the list of range data to look for the smallest suitable
  // range.
  cur_range_item = vmm_range_data_list.head;
  while(cur_range_item != NULL)
  {
    //KL_TRC_TRACE((TRC_LVL_FLOW, "Looking at next item.\n"));
    cur_range = (vmm_range_data *)cur_range_item->item;
    ASSERT(cur_range != NULL);
    if ((cur_range->allocated == false) &&
        (cur_range->number_of_pages >= num_pages) &&
        ((selected_range == NULL) ||
         (selected_range->number_of_pages > cur_range->number_of_pages)))
    {
      //KL_TRC_TRACE((TRC_LVL_FLOW, "Item found.\n"));
      selected_range = cur_range;
      selected_range_item = cur_range_item;
    }
    cur_range_item = cur_range_item->next;
  }

  KL_TRC_EXIT;

  // TODO: Do something more sensible for out-of-memory conditions. (STAB)
  ASSERT(selected_range_item != NULL);
  return selected_range_item;
}

// Split a large memory range in to two or more smaller chunks, so as to be left
// with at least one that's exactly number_of_pages_reqd in length.
klib_list_item *mem_vmm_split_range(klib_list_item *item_to_split,
                                    unsigned int number_of_pages_reqd)
{
  KL_TRC_ENTRY;

  klib_list_item *second_half_of_split;
  vmm_range_data *old_range_data;
  vmm_range_data *new_range_data;

  // Allocate a new list item and range data. Use these special functions since
  // VMM manages its own memory.
  second_half_of_split = mem_vmm_allocate_list_item();
  new_range_data = mem_vmm_allocate_range_item();

  // Add the new range to the list of ranges after the old one. We'll always
  // pass back the first half of the pair.
  second_half_of_split->item = (void *)new_range_data;
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
    item_to_split = mem_vmm_split_range(item_to_split, number_of_pages_reqd);
  }

  KL_TRC_EXIT;

  return item_to_split;
}

// See whether a recently freed range can be merged with its partner. If it can,
// repeat the process for the newly merged range.
void mem_vmm_resolve_merges(klib_list_item *start_point)
{
  KL_TRC_ENTRY;

  vmm_range_data *this_data;
  klib_list_item *partner_item;
  vmm_range_data *partner_data;
  bool first_half_of_pair;
  unsigned long next_block_size;

  klib_list_item *survivor_item;
  vmm_range_data *survivor_data;
  klib_list_item *released_item;
  vmm_range_data *released_data;

  ASSERT (start_point != NULL);

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

// Allocate a specific range of virtual memory. This is primarily used when
// setting up VMM. num_pages must be a power of two, and start_addr must start
// on a memory address that's a multiple of num_pages.
void mem_vmm_allocate_specific_range(unsigned long start_addr,
                                     unsigned int num_pages)
{
  KL_TRC_ENTRY;

  klib_list_item *cur_item;
  vmm_range_data *cur_data;
  unsigned long end_addr;
  unsigned int rounded_num_pages = round_to_power_two(num_pages);

  // Check that start_addr is on a boundary that matches the number of pages
  // requested.
  ASSERT(rounded_num_pages == num_pages);
  ASSERT((start_addr % (num_pages * MEM_PAGE_SIZE)) == 0);

  // Look for the range that contains this memory address. Split it down to
  // size.
  cur_item = vmm_range_data_list.head;
  while (cur_item != NULL)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "cur_item != NULL\n"));
    cur_data = (vmm_range_data *)cur_item->item;

    // Have end_addr be one lower than it seems like it ought to be. This
    // prevents any issues with ranges that end exactly at the top of RAM, and
    // will never interfere with the calculation (since pages will always be
    // larger than 1 byte in size!)
    end_addr = cur_data->start + (cur_data->number_of_pages * MEM_PAGE_SIZE) -1;
    if ((cur_data->start <= start_addr) && (end_addr > start_addr))
    {
      KL_TRC_TRACE((TRC_LVL_FLOW, "Correct range found\n"));
      ASSERT(cur_data->number_of_pages >= num_pages);

      // If the range we've found is the correct size - perfect. Allocate it and
      // carry on. Otherwise it must be too large. Split it in two and try again
      // to allocate it.
      if (cur_data->number_of_pages == num_pages)
      {
        KL_TRC_TRACE((TRC_LVL_FLOW, "Correct size found\n"));
        ASSERT(!cur_data->allocated);
        cur_data->allocated = true;
      }
      else
      {
        KL_TRC_TRACE((TRC_LVL_FLOW, "Size too large\n"));
        cur_item = mem_vmm_split_range(cur_item, cur_data->number_of_pages / 2);

        // Recursion isn't the most efficient way to deal with this, but it'll
        // do for now. TODO: Improve efficiency.
        mem_vmm_allocate_specific_range(start_addr, num_pages);
      }

      KL_TRC_EXIT;
      return;
    }

    cur_item = cur_item->next;
  }

  KL_TRC_TRACE((TRC_LVL_FLOW, "Item found\n"));

  // Presumably this means we tried to get a range that's not owned by the
  // kernel.
  ASSERT(cur_item != NULL);

  KL_TRC_EXIT;
}

//------------------------------------------------------------------------------
// Internal memory management code.
//------------------------------------------------------------------------------

// Note the similarity with mem_vmm_allocate_range_item.
klib_list_item *mem_vmm_allocate_list_item()
{
  KL_TRC_ENTRY;

  klib_list_item *ret_item;

  // Use one of the preallocated "initial_range_list" items if any are left.
  // There should be enough to last until VMM is fully initialised, at which
  // point grabbing them from kmalloc should be fine.
  if (initial_list_items_used >= NUM_INITIAL_RANGES)
  {
    ASSERT(vmm_initialized);
    ret_item = (klib_list_item *)kmalloc(sizeof(klib_list_item));
  }
  else
  {
    ret_item = &initial_range_list[initial_list_items_used];
    initial_list_items_used++;
  }

  KL_TRC_EXIT;

  return ret_item;
}

// Note the similarity with mem_vmm_allocate_list_item.
vmm_range_data *mem_vmm_allocate_range_item()
{
  KL_TRC_ENTRY;

  vmm_range_data *ret_item;

  // Use one of the preallocated "initial_range_list" items if any are left.
  // There should be enough to last until VMM is fully initialised, at which
  // point grabbing them from kmalloc should be fine.
  if (initial_ranges_used >= NUM_INITIAL_RANGES)
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

// Note the similarity with mem_vmm_free_range_item.
void mem_vmm_free_list_item (klib_list_item *item)
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

// Note the similarity with mem_vmm_free_list_item.
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
