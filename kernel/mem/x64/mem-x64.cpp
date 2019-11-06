/// @file
/// @brief x64-specific Memory Management Code
///
/// The bulk of x64-specific code deals with managing the page tables.
///
/// Each process has its own complete set of page tables. However, the kernel section (all addresses above the
/// mid-point in memory) is kept synchronised across all processes, by updating the PML4 for each process whenever the
/// PML4 entries relevant to the kernel are altered. At present, deallocating virtual ranges only unsets the PTEs, not
/// the PDEs or PML4 entries, so this only happens during range allocation.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/x64/processor-x64.h"
#include "mem/mem.h"
#include "mem/mem-int.h"
#include "mem/x64/mem-x64.h"
#include "mem/x64/mem-x64-int.h"

mem_process_info task0_entry; ///< Memory information for the kernel 'process'
process_x64_data task0_x64_entry; ///< x64-specific memory information for the kernel 'process'

/// This pointer points to the location in virtual address space corresponding to the physical page being used as the
/// PTE for working_table_virtual_addr. This means that by writing to working_table_va_entry_addr,
/// working_table_virtual_addr can be changed to point at a different physical page. This is calculated by the assembly
/// language entry code.
extern "C" uint64_t *working_table_va_entry_addr;
uint64_t *working_table_va_entry_addr;

namespace
{
  /// This mask represents the bits that are valid in a physical address - i.e., limited by MAXPHYADDR. See the Intel
  /// System Programming Guide chapter 4.1.4 for details. It is populated at runtime.
  uint64_t valid_phys_bit_mask = 0;

  /// When editing page tables, they are not necessarily mapped in the kernel's address space, so this provides a
  /// constant virtual address to use when working on page tables. This is the base address of the 2MB page.
  const uint64_t working_table_virtual_addr_base = 0xFFFFFFFFFFE00000;

  /// This will always be within 2MB of working_table_virtual_addr_base, and points to the specific page table being
  /// edited, rather than a whole 2MB page of them.
  uint64_t working_table_virtual_addr;

  /// Azalea uses 2MB pages, but page tables are 4kB in size. This points to the next 4kB page to allocate when
  /// allocating page tables.
  uint8_t *next_4kb_page;

  /// Is the table currently being edited mapped to kernel space?
  bool working_table_va_mapped;

  /// A lock to protect writes to the list of PML4 tables.
  static kernel_spinlock pml4_edit_lock;

  void *mem_get_next_4kb_page();
  uint8_t mem_x64_get_max_phys_addr();
}

/// @brief Initialise the entire memory management subsystem.
///
/// This function is required across all platforms. However, the bulk of it is x64 specific, so it lives here.
///
/// @param e820_ptr Pointer to the E820 memory map provided by the bootloader.
void mem_gen_init(e820_pointer *e820_ptr)
{
  KL_TRC_ENTRY;

  uint64_t temp_phys_addr;
  uint64_t temp_offset;
  uint8_t phys_addr_width;

  // Initialise the physical memory subsystem. This will call back to x64- specific code later.
  mem_init_gen_phys_sys(e820_ptr);

  // Configure the x64 PAT system, so that caching works as expected.
  mem_x64_pat_init();

  // Setup the page use counter table, so that pages physical pages can be automatically released.
  mem_map_init_counters();

  // Determine the maximum physical address length, and as a result a bit mask that can be used to mask out invalid
  // bits.
  phys_addr_width = mem_x64_get_max_phys_addr();
  valid_phys_bit_mask = 1;
  valid_phys_bit_mask = valid_phys_bit_mask << phys_addr_width;
  valid_phys_bit_mask -= 1;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Physical address bit mask: ", valid_phys_bit_mask, "\n");

  klib_synch_spinlock_init(pml4_edit_lock);

  // Prepare the virtual memory subsystem. Start with some fairly simple initialisation.
  task0_x64_entry.pml4_phys_addr = (uint64_t)&pml4_table;
  task0_x64_entry.pml4_virt_addr = task0_x64_entry.pml4_phys_addr + 0xFFFFFFFF00000000;
  task0_entry.arch_specific_data = (void *)&task0_x64_entry;
  mem_x64_pml4_init_sys(task0_x64_entry);

  temp_offset = task0_x64_entry.pml4_virt_addr % MEM_PAGE_SIZE;
  temp_phys_addr = (uint64_t)mem_get_phys_addr((void *)(task0_x64_entry.pml4_virt_addr - temp_offset));
  ASSERT(temp_phys_addr == (task0_x64_entry.pml4_phys_addr - temp_offset));

  next_4kb_page = nullptr;
  working_table_va_mapped = false;

  KL_TRC_EXIT;
}

/// @brief Generate the bitmap of physical pages, for use in the physical memory manager.
///
/// The number of physical pages in the system cannot exceed max_num_pages, or the system will crash.
///
/// @param e820_ptr Pointer to an E820 table given to us by the bootloader (or other means)
///
/// @param bitmap_loc Where the physical memory manager (which is not platform-specific) requires the bitmap to be
///                   stored.
///
/// @param max_num_pages The maximum number of pages the physical memory manager can deal with. IF the number of pages
///                      available to the system exceeds this, the system will crash.
void mem_gen_phys_pages_bitmap(e820_pointer *e820_ptr, uint64_t *bitmap_loc, uint64_t max_num_pages)
{
  KL_TRC_ENTRY;

  // Spin through the E820 memory map and look at the ranges to determine whether they're usable or not.
  const e820_record *cur_record;
  uint64_t start_addr;
  uint64_t end_addr;
  uint64_t number_of_pages;
  uint64_t bytes_read = 0;

  static_assert(sizeof(e820_record) == 24, "E820 record struct has been wrongly edited");

  ASSERT(e820_ptr != nullptr);
  ASSERT(bitmap_loc != nullptr);
  ASSERT(e820_ptr->table_ptr != nullptr);
  ASSERT(e820_ptr->table_length >= sizeof(e820_record));

  KL_TRC_TRACE(TRC_LVL::FLOW, "E820 Map Location: ", e820_ptr->table_ptr, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "E820 Map Length: ", e820_ptr->table_length, "\n");

  cur_record = e820_ptr->table_ptr;

  // Set the bitmap to 0 - i.e. unallocated.
  kl_memset(bitmap_loc, 0, max_num_pages / 8);

  while (((cur_record->start_addr != 0) ||
         (cur_record->length != 0) ||
         (cur_record->memory_type != 0)) &&
         (bytes_read < e820_ptr->table_length))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW,
                 "Record. Start: ", cur_record->start_addr,
                 ", length: ", cur_record->length,
                 ", type: ", cur_record->memory_type, "\n");
    // Only type 1 memory is usable. If we've found some, round the start and end addresses to 2MB boundaries. Always
    // ignore the first 2MB of RAM - we've already loaded the kernel in to there and it has some crazy stuff in anyway.
    //
    // We assume that the VGA buffers, etc. always mean there's a block within the first 2MB that's allocated.
    if (cur_record->memory_type == 1)
    {
      start_addr = cur_record->start_addr;
      if (start_addr % MEM_PAGE_SIZE != 0)
      {
        start_addr = start_addr + MEM_PAGE_SIZE - (start_addr % MEM_PAGE_SIZE);
      }
      end_addr = cur_record->start_addr + cur_record->length;
      if (end_addr % MEM_PAGE_SIZE != 0)
      {
        end_addr = end_addr - (end_addr % MEM_PAGE_SIZE);
      }

      // Only keep considering this chunk of pages if the start and end addresses are in the correct order. They might
      // not be if this record points to a small chunk of RAM in the middle of a 2MB block.
      //
      // As mentioned above, ignore the zero-2MB RAM page - this already has the kernel loaded in to it.
      if (end_addr > start_addr)
      {
        number_of_pages = (end_addr - start_addr) / MEM_PAGE_SIZE;
        ASSERT(number_of_pages != 0);
        for (int i = 0; i < number_of_pages; i++)
        {
          if (start_addr != 0)
          {
            mem_set_bitmap_page_bit(start_addr, true);
          }
          start_addr += MEM_PAGE_SIZE;
        }
      }
    }
    cur_record++;
    bytes_read += sizeof(e820_record);
  }

  KL_TRC_EXIT;
}

/// @brief Map a single virtual page to a single physical page
///
/// @param virt_addr The virtual address that requires mapping
///
/// @param phys_addr The physical address that will be backing virt_addr
///
/// @param context The process that the mapping should occur in. Defaults to the currently running process.
///
/// @param cache_mode Which cache mode is required. Defaults to WRITE_BACK.
void mem_x64_map_virtual_page(uint64_t virt_addr,
                              uint64_t phys_addr,
                              task_process *context,
                              MEM_CACHE_MODES cache_mode)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Requested (virtual)", virt_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Requested (physical)", phys_addr, "\n");

  uint64_t *table_addr = get_pml4_table_addr(context);
  uint64_t virt_addr_cpy = virt_addr;
  uint64_t pml4_entry_idx;
  uint64_t page_dir_ptr_entry_idx;
  uint64_t page_dir_entry_idx;
  uint64_t *encoded_entry;
  void *table_phys_addr;
  page_table_entry new_entry;
  bool is_kernel_allocation;

  // Truncate the physical address to be limited by MAXPHYADDR
  ASSERT(valid_phys_bit_mask != 0);
  phys_addr = phys_addr & valid_phys_bit_mask;

  is_kernel_allocation = ((virt_addr & 0x8000000000000000) != 0);

  virt_addr_cpy = virt_addr_cpy >> 21;
  page_dir_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  page_dir_ptr_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  pml4_entry_idx = (virt_addr_cpy & 0x00000000000001FF);

  // Generate or check the PML4 address.
  encoded_entry = table_addr + pml4_entry_idx;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "PML4 Index", pml4_entry_idx, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Table address", table_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Encoded entry addr", encoded_entry, "\n");
  if (PT_MARKED_PRESENT(*encoded_entry))
  {
    // Get the physical address of the next table.
    KL_TRC_TRACE(TRC_LVL::FLOW, "PML4 entry marked present\n");
    table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "PML4 entry not present\n");
    table_phys_addr = (void *)mem_get_next_4kb_page();

    new_entry.target_addr = (uint64_t)table_phys_addr;
    new_entry.present = true;
    new_entry.writable = true;
    new_entry.user_mode = !is_kernel_allocation;
    new_entry.end_of_tree = false;
    new_entry.cache_type = MEM_X64_CACHE_TYPES::WRITE_BACK;

    if (is_kernel_allocation)
    {
      klib_synch_spinlock_lock(pml4_edit_lock);
    }

    *encoded_entry = mem_encode_page_table_entry(new_entry);

    // If this allocation relates to the kernel - that is, it is for an allocation in the upper-half of memory, we need
    // to synchronise the relevant PML4s across all processes.
    if (is_kernel_allocation)
    {
      ASSERT(pml4_entry_idx >= 2048);
      KL_TRC_TRACE(TRC_LVL::FLOW, "Synchronizing PML4.\n");
      mem_x64_pml4_synchronize((void *)table_addr);
      klib_synch_spinlock_unlock(pml4_edit_lock);
    }
  }

  // Now look at the page directory pointer table. This is temporarily mapped to a well-known virtual address, since
  // there's no direct mapping back from physical address to addresses accessible by the kernel.
  mem_set_working_page_dir((uint64_t)table_phys_addr);
  table_addr = (uint64_t *)working_table_virtual_addr;
  encoded_entry = table_addr + page_dir_ptr_entry_idx;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "PDPT Index", page_dir_ptr_entry_idx, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Table address (phys)", table_phys_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Encoded entry addr" , encoded_entry, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Encoded entry", *encoded_entry, "\n");
  if (PT_MARKED_PRESENT(*encoded_entry))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "PDPT entry marked present\n");
    table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "PDPT entry not present\n");

    table_phys_addr = (void *)mem_get_next_4kb_page();

    new_entry.target_addr = (uint64_t)table_phys_addr;
    new_entry.present = true;
    new_entry.writable = true;
    new_entry.user_mode = !is_kernel_allocation;
    new_entry.end_of_tree = false;
    new_entry.cache_type = MEM_X64_CACHE_TYPES::WRITE_BACK;

    *encoded_entry = mem_encode_page_table_entry(new_entry);
    KL_TRC_TRACE(TRC_LVL::EXTRA, "New entry", *encoded_entry, "\n");
  }

  // Having mapped the page directory, it's possible to map the physical address to a virtual address. To prevent
  // kernel bugs, assert that it's not already present - this'll stop any accidental overwriting of in-use page table
  // entries.
  mem_set_working_page_dir((uint64_t)table_phys_addr);
  table_addr = (uint64_t *)working_table_virtual_addr;
  encoded_entry = table_addr + page_dir_entry_idx;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Page dir Index", (uint64_t) page_dir_entry_idx, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "table_addr", (uint64_t)table_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "encoded_entry addr", (uint64_t)encoded_entry, "\n");
  ASSERT(!PT_MARKED_PRESENT(*encoded_entry));

  new_entry.target_addr = phys_addr;
  new_entry.present = true;
  new_entry.writable = true;
  new_entry.user_mode = !is_kernel_allocation;
  new_entry.end_of_tree = true;
  new_entry.cache_type = (uint8_t)cache_mode;
  *encoded_entry = mem_encode_page_table_entry(new_entry);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Encoded entry", (uint64_t)*encoded_entry, "\n");

  KL_TRC_EXIT;
}

/// @brief Break the connection between a virtual memory address and its physical backing.
///
/// @param virt_addr The virtual memory address that will become unmapped.
///
/// @param context The process to do the unmapping in.
void mem_x64_unmap_virtual_page(uint64_t virt_addr, task_process *context)
{
  KL_TRC_ENTRY;

  uint64_t virt_addr_cpy = virt_addr;
  uint64_t pml4_entry_idx;
  uint64_t page_dir_ptr_entry_idx;
  uint64_t page_dir_entry_idx;
  uint64_t *table_addr = get_pml4_table_addr(context);
  uint64_t *encoded_entry;
  void *table_phys_addr;

  virt_addr_cpy = virt_addr_cpy >> 21;
  page_dir_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  page_dir_ptr_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  pml4_entry_idx = (virt_addr_cpy & 0x00000000000001FF);

  // Start moving through the page table tree by looking at the PML4 table.
  encoded_entry = table_addr + pml4_entry_idx;
  if (PT_MARKED_PRESENT(*encoded_entry))
  {
    // Get the physical address of the next table.
    table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);
  }
  else
  {
    // Presumably it isn't already mapped, so bail out.
    KL_TRC_EXIT;
    return;
  }

  // Now look at the page directory pointer table.
  mem_set_working_page_dir((uint64_t)table_phys_addr);
  table_addr = (uint64_t *)working_table_virtual_addr;
  encoded_entry = table_addr + page_dir_ptr_entry_idx;
  if (PT_MARKED_PRESENT(*encoded_entry))
  {
    table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);
  }
  else
  {
    // Presumably the address is unmapped, return.
    KL_TRC_EXIT;
    return;
  }

  // Having mapped the page directory, it's possible to unmap the range by setting the entry to NULL.
  mem_set_working_page_dir((uint64_t)table_phys_addr);
  table_addr = (uint64_t *)working_table_virtual_addr;
  encoded_entry = table_addr + page_dir_entry_idx;
  *encoded_entry = 0;

  // We now need to flush this page table.
  mem_invalidate_page_table(virt_addr);

  KL_TRC_EXIT;
}

namespace
{
  /// @brief Return the physical address of a 4kB "page" usable by the page table system by neatly carving up a 2MB
  /// page.
  ///
  /// This is useful for certain callers instead of calling mem_allocate_phys_page because that returns 2MB pages which
  /// results in huge wasteage.
  ///
  /// @return The physical address of the beginning of a 4kB "page".
  void *mem_get_next_4kb_page()
  {
    KL_TRC_ENTRY;

    void *ret_val;
    if (next_4kb_page == nullptr)
    {
      next_4kb_page = (uint8_t *)mem_allocate_physical_pages(1);
    }

    ret_val = next_4kb_page;

    next_4kb_page = next_4kb_page + 4096;

    if (((uint64_t)next_4kb_page) % MEM_PAGE_SIZE == 0)
    {
      next_4kb_page = nullptr;
    }

    KL_TRC_EXIT;

    return ret_val;
  }
}

/// @brief Set up a well-known virtual address to the given physical address.
///
/// @param phys_page_addr The physical page that needs mapping to working_table_va_entry_addr
void mem_set_working_page_dir(uint64_t phys_page_addr)
{
  KL_TRC_ENTRY;

  page_table_entry new_entry;
  uint64_t page_offset;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "phys_page_addr", phys_page_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "working_table_va_entry_addr", working_table_va_entry_addr, "\n");

  ASSERT(working_table_va_entry_addr != nullptr);
  ASSERT((phys_page_addr & 0x0FFF) == 0);

  page_offset = phys_page_addr & 0x1FFFFF;
  phys_page_addr = phys_page_addr & ~((uint64_t)0x1FFFFF);

  if (working_table_va_mapped)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalidating PT\n");
    *working_table_va_entry_addr = 0;
    mem_invalidate_page_table(working_table_virtual_addr_base);
    working_table_va_mapped = false;
  }

  new_entry.target_addr = phys_page_addr;
  new_entry.present = true;
  new_entry.writable = true;
  new_entry.user_mode = false;
  new_entry.end_of_tree = true;
  new_entry.cache_type = MEM_X64_CACHE_TYPES::WRITE_BACK;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "working_table_va_entry_addr", working_table_va_entry_addr, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "*working_table_va_entry_addr", *working_table_va_entry_addr, "\n");

  *working_table_va_entry_addr = mem_encode_page_table_entry(new_entry);
  mem_invalidate_page_table(working_table_virtual_addr_base);
  working_table_virtual_addr = working_table_virtual_addr_base + page_offset;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "page_offset", page_offset, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "working_table_virtual_addr", working_table_virtual_addr, "\n");

  working_table_va_mapped = true;

  KL_TRC_EXIT;
}

/// @brief Encode a page table entry from a nice user-friendly struct.
///
/// Converts the struct given as an argument into the format used by x64 processors.
///
/// @param pte The page table entry (in struct format) that needs converting into machine format.
///
/// @return The encoded version of the PTE structure.
uint64_t mem_encode_page_table_entry(page_table_entry &pte)
{
  KL_TRC_ENTRY;

  uint8_t pat_value;

  uint64_t masked_addr = pte.target_addr & 0x0007FFFFFFFFF000;
  uint64_t result = masked_addr |
      (pte.end_of_tree ? 0x80 : 0x00) |
      (pte.present ? 0x01 : 0x00) |
      (pte.writable ? 0x02 : 0x00) |
      (pte.user_mode ? 0x04 : 0x00);

  pat_value = mem_x64_pat_get_val(pte.cache_type, !pte.end_of_tree);
  ASSERT((!pte.end_of_tree) | (pat_value < 4));
  ASSERT((!pte.end_of_tree) | ((pte.target_addr & 0x00000000000FF000) == 0));

  // Encode the cache type into PAT (bit 12), PCD (bit 4) and PWT (bit 3), per the Intel System Programming Guide,
  // section 4.9.2.
  //
  // Entries in the tree that reference another part of the tree (i.e. they don't point at the translated address) do
  // not have a PAT field, which is why their PAT index must be less than 4.
  //
  // We can get away with assuming the PAT to be in bit 12, because we never allocate pages less than 2MB.
  result = result | ((pat_value & 0x03) << 3);
  if ((pte.end_of_tree) && ((pat_value & 0x04) != 0))
  {
    result = result | 0x1000;
  }

  KL_TRC_EXIT;

  return result;
}

/// @brief Decode an encoded page table entry back to a nice user-friendly struct.
///
/// @param encoded The encoded page table entry, as used by the system
///
/// @return The structure format version of the PTE.
page_table_entry mem_decode_page_table_entry(uint64_t encoded)
{
  KL_TRC_ENTRY;

  page_table_entry decode;
  uint8_t pat_val;

  decode.end_of_tree = ((encoded & 0x80) != 0);
  decode.present = ((encoded & 0x01) != 0);
  decode.writable = ((encoded & 0x02) != 0);
  decode.user_mode = ((encoded & 0x04) != 0);

  pat_val = (encoded & 0x18) >> 3;
  if (decode.end_of_tree)
  {
    if ((encoded & 0x1000) != 0)
    {
      pat_val = pat_val | 0x04;
    }
  }

  decode.cache_type = mem_x64_pat_decode(pat_val);

  // The number of bits allocated to the memory address changes depending on whether this is at the end of the
  // translation tree or not. Assuming all but the bottom 12 bits are part of the address doesn't take into account the
  // PAT bit that sits at bit 12.
  if (decode.end_of_tree)
  {
    decode.target_addr = encoded & 0x0007FFFFFFF00000;
  }
  else
  {
    decode.target_addr = encoded & 0x0007FFFFFFFFF000;
  }

  KL_TRC_EXIT;

  return decode;
}

/// @brief For a given virtual address, find the physical address that backs it.
///
/// @param virtual_addr The virtual address to decode. Need not point at a page boundary.
///
/// @param context The process context to do this decoding in. If nullptr is supplied, use the current context.
///
/// @return The physical address backing virtual_addr, or nullptr if no physical RAM backs virtual_addr
void *mem_get_phys_addr(void *virtual_addr, task_process *context)
{
  KL_TRC_ENTRY;

  uint64_t virt_addr_cpy;
  uint64_t pml4_entry_idx;
  uint64_t page_dir_ptr_entry_idx;
  uint64_t page_dir_entry_idx;
  uint64_t *table_addr = get_pml4_table_addr(context);
  uint64_t *encoded_entry;
  void *table_phys_addr;
  uint64_t offset;
  bool return_addr_found = false;
  uint64_t phys_addr = 0;

  offset = ((uint64_t)virtual_addr) % MEM_PAGE_SIZE;
  virt_addr_cpy = ((uint64_t)virtual_addr) - offset;

  virt_addr_cpy = virt_addr_cpy >> 21;
  page_dir_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  page_dir_ptr_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  pml4_entry_idx = (virt_addr_cpy & 0x00000000000001FF);

  // Start moving through the page table tree by looking at the PML4 table and
  // getting the physical address of the next table.
  encoded_entry = table_addr + pml4_entry_idx;
  if (PT_MARKED_PRESENT(*encoded_entry))
  {
    table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);

    // Now look at the page directory pointer table.
    mem_set_working_page_dir((uint64_t)table_phys_addr);
    table_addr = (uint64_t *)working_table_virtual_addr;
    encoded_entry = table_addr + page_dir_ptr_entry_idx;
    if (PT_MARKED_PRESENT(*encoded_entry))
    {
      table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);

      // Having worked through all the page directories, grab the address out.
      mem_set_working_page_dir((uint64_t)table_phys_addr);
      table_addr = (uint64_t *)working_table_virtual_addr;
      encoded_entry = table_addr + page_dir_entry_idx;

      phys_addr = mem_x64_phys_addr_from_pte(*encoded_entry);
      phys_addr += offset;
      return_addr_found = true;
    }
  }

  KL_TRC_EXIT;

  return return_addr_found ? reinterpret_cast<void *>(phys_addr) : nullptr;
}

/// @brief Get the virtual address of the PML4 table for the currently running process.
///
/// Get the PML4 table address for the selected process, or the currently running process if context = NULL. The only
/// niggle is that there might not be a running thread if we're still sorting out memory before the task manager
/// starts. So, in that case, provide the address for the kernel table from the knowledge we set up during
/// initialisation.
///
/// @param context The process to get the PML4 address for. May be NULL, in which case return it for the current
///                process.
///
/// @return the address of the PML4 table.
uint64_t *get_pml4_table_addr(task_process *context)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Context", context, "\n");

  task_thread *cur_thread = task_get_cur_thread();
  task_process *cur_process;
  mem_process_info *mem_info;
  process_x64_data *proc_data;
  uint64_t *table_addr;

  if (context != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Context provided, use appropriate PML4\n");
    mem_info = context->mem_info;
    ASSERT(mem_info != nullptr);
    proc_data = (process_x64_data *)mem_info->arch_specific_data;
    ASSERT(proc_data != nullptr);
    table_addr = (uint64_t *)proc_data->pml4_virt_addr;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No context provided, use current context\n");
    if(cur_thread != nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Provide process specific data\n");
      cur_process = cur_thread->parent_process.get();
      ASSERT(cur_process != nullptr);
      mem_info = cur_process->mem_info;
      ASSERT(mem_info != nullptr);
      proc_data = (process_x64_data *)mem_info->arch_specific_data;
      ASSERT(proc_data != nullptr);
      table_addr = (uint64_t *)proc_data->pml4_virt_addr;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No context provided, use current context\n");
      table_addr = (uint64_t *)task0_x64_entry.pml4_virt_addr;
    }
  }

  ASSERT(table_addr != nullptr);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Returning PML4 address", table_addr, "\n");

  KL_TRC_EXIT;
  return table_addr;
}

/// @brief Convert an encoded PTE into the physical address backing it.
///
/// @param encoded An encoded PTE
///
/// @return The physical backing address.
uint64_t mem_x64_phys_addr_from_pte(uint64_t encoded)
{
  KL_TRC_ENTRY;

  page_table_entry decoded = mem_decode_page_table_entry(encoded);

  KL_TRC_EXIT;

  return decoded.target_addr;
}

/// @brief Determine whether a provided virtual address is valid.
///
/// On the x64 architecture this basically means "is it canonical". For a definition of canonical, see the Intel
/// Software Developer's Manual Volume 1, Chapter 3.3.7.1. We assume a 48-bit implementation of the architecture.
///
/// @param virtual_addr The address to test for validity.
///
/// @return true if the address is canonical, false otherwise.
bool mem_is_valid_virt_addr(uint64_t virtual_addr)
{
  KL_TRC_ENTRY;

  bool result;
  const uint64_t mask = 0xFFFF000000000000;

  result = ((virtual_addr & mask) == 0) || ((virtual_addr & mask) == mask);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

namespace
{
  /// @brief Determine the value of MAXPHYADDR
  ///
  /// Determine the width of physical addresses the processor uses. For a better description see, in particular, the
  /// Intel System Programming Guide chapter 4.1.4.
  ///
  /// @return A value corresponding to MAXPHYADDR - the number of bits the processor uses in physical addresses.
  uint8_t mem_x64_get_max_phys_addr()
  {
    uint8_t result;
    uint64_t ebx_eax;
    uint64_t edx_ecx;

    KL_TRC_ENTRY;

    // Request the memory information leaf.
    asm_proc_read_cpuid(0x80000008, 0, &ebx_eax, &edx_ecx);
    result = static_cast<uint8_t>(ebx_eax & 0xFF);

    ASSERT((result > 35) && (result <= 52)); // Per the current Intel documentation.

    KL_TRC_TRACE(TRC_LVL::EXTRA, "MAXPHYADDR: ", result, "\n");
    KL_TRC_EXIT;

    return result;
  }
}