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
#include "mem/mem.h"
#include "mem/mem-int.h"
#include "mem/x64/mem-x64.h"
#include "mem/x64/mem-x64-int.h"

mem_process_info task0_entry;
process_x64_data task0_x64_entry;

const unsigned long working_table_virtual_addr_base = 0xFFFFFFFFFFE00000;
unsigned long working_table_virtual_addr;

// This pointer points to the location in virtual address space corresponding to
// the physical page being used as the PTE for working_table_virtual_addr. This
// means that by writing to working_table_va_entry_addr,
// working_table_virtual_addr can be changed to point at a different physical
// page. This is calculated by the assembly language entry code.
extern "C" unsigned long *working_table_va_entry_addr;
unsigned long *working_table_va_entry_addr;

// This pointer is the virtual address of the kernel stack. Its physical address will be different in every process,
// but its virtual address will always be the same (so it can be filled in to the x64 TSS).
void *mem_x64_kernel_stack_ptr;

unsigned char *next_4kb_page;
bool working_table_va_mapped;

static kernel_spinlock pml4_edit_lock;

void *mem_get_next_4kb_page();

/// @brief Initialise the entire memory management subsystem.
///
/// This function is required across all platforms. However, the bulk of it is x64 specific, so it lives here.
void mem_gen_init(e820_pointer *e820_ptr)
{
  KL_TRC_ENTRY;

  unsigned long temp_phys_addr;
  unsigned long temp_offset;

  // Initialise the physical memory subsystem. This will call back to x64- specific code later.
  mem_init_gen_phys_sys(e820_ptr);

  // Configure the x64 PAT system, so that caching works as expected.
  mem_x64_pat_init();

  klib_synch_spinlock_init(pml4_edit_lock);

  // Prepare the virtual memory subsystem. Start with some fairly simple initialisation.
  task0_x64_entry.pml4_phys_addr = (unsigned long)&pml4_table;
  task0_x64_entry.pml4_virt_addr = task0_x64_entry.pml4_phys_addr + 0xFFFFFFFF00000000;
  task0_entry.arch_specific_data = (void *)&task0_x64_entry;
  mem_x64_pml4_init_sys(task0_x64_entry);

  temp_offset = task0_x64_entry.pml4_virt_addr % MEM_PAGE_SIZE;
  temp_phys_addr = (unsigned long)mem_get_phys_addr((void *)(task0_x64_entry.pml4_virt_addr - temp_offset));
  ASSERT(temp_phys_addr == (task0_x64_entry.pml4_phys_addr - temp_offset));

  next_4kb_page = nullptr;
  working_table_va_mapped = false;

  // Allocate a virtual address that is used for the kernel stack in all processes.
  mem_x64_kernel_stack_ptr = mem_allocate_virtual_range(1);

  // At the minute, all process actually just use the same stack, so back that up with a physical page.
  mem_map_range(mem_allocate_physical_pages(1), mem_x64_kernel_stack_ptr, 1);

  KL_TRC_EXIT;
}

/// @brief Generate the bitmap of physical pages, for use in the physical memory manager.
///
/// The number of physical pages in the system cannot exceed max_num_pages, or the system will crash.
///
/// @param bitmap_loc Where the physical memory manager (which is not platform-specific) requires the bitmap to be
///                   stored.
///
/// @param max_num_pages The maximum number of pages the physical memory manager can deal with. IF the number of pages
///                      available to the system exceeds this, the system will crash.
void mem_gen_phys_pages_bitmap(e820_pointer *e820_ptr, unsigned long *bitmap_loc, unsigned long max_num_pages)
{
  KL_TRC_ENTRY;

  // Spin through the E820 memory map and look at the ranges to determine whether they're usable or not.
  const e820_record *cur_record;
  unsigned long start_addr;
  unsigned long end_addr;
  unsigned long number_of_pages;
  unsigned long bytes_read = 0;

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
void mem_map_virtual_page(unsigned long virt_addr,
                          unsigned long phys_addr,
                          task_process *context,
                          MEM_CACHE_MODES cache_mode)
{
  KL_TRC_ENTRY;

  KL_TRC_DATA("Requested (virtual)", virt_addr);
  KL_TRC_DATA("Requested (physical)", phys_addr);

  unsigned long *table_addr = get_pml4_table_addr(context);
  unsigned long virt_addr_cpy = virt_addr;
  unsigned long pml4_entry_idx;
  unsigned long page_dir_ptr_entry_idx;
  unsigned long page_dir_entry_idx;
  unsigned long *encoded_entry;
  void *table_phys_addr;
  page_table_entry new_entry;
  bool is_kernel_allocation;

  is_kernel_allocation = ((virt_addr & 0x8000000000000000) != 0);

  virt_addr_cpy = virt_addr_cpy >> 21;
  page_dir_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  page_dir_ptr_entry_idx = (virt_addr_cpy & 0x00000000000001FF);
  virt_addr_cpy = virt_addr_cpy >> 9;
  pml4_entry_idx = (virt_addr_cpy & 0x00000000000001FF);

  // Generate or check the PML4 address.
  encoded_entry = table_addr + pml4_entry_idx;
  KL_TRC_DATA("PML4 Index", (unsigned long) pml4_entry_idx);
  KL_TRC_DATA("Table address", (unsigned long) table_addr);
  KL_TRC_DATA("Encoded entry addr", (unsigned long) encoded_entry);
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

    new_entry.target_addr = (unsigned long)table_phys_addr;
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
  mem_set_working_page_dir((unsigned long)table_phys_addr);
  table_addr = (unsigned long *)working_table_virtual_addr;
  encoded_entry = table_addr + page_dir_ptr_entry_idx;
  KL_TRC_DATA("PDPT Index", (unsigned long) page_dir_ptr_entry_idx);
  KL_TRC_DATA("Table address (phys)", (unsigned long) table_phys_addr);
  KL_TRC_DATA("Encoded entry addr" , (unsigned long) encoded_entry);
  KL_TRC_DATA("Encoded entry", (unsigned long) *encoded_entry);
  if (PT_MARKED_PRESENT(*encoded_entry))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "PDPT entry marked present\n");
    table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "PDPT entry not present\n");

    table_phys_addr = (void *)mem_get_next_4kb_page();

    new_entry.target_addr = (unsigned long)table_phys_addr;
    new_entry.present = true;
    new_entry.writable = true;
    new_entry.user_mode = !is_kernel_allocation;
    new_entry.end_of_tree = false;
    new_entry.cache_type = MEM_X64_CACHE_TYPES::WRITE_BACK;

    *encoded_entry = mem_encode_page_table_entry(new_entry);
    KL_TRC_DATA("New entry", *encoded_entry);
  }

  // Having mapped the page directory, it's possible to map the physical address to a virtual address. To prevent
  // kernel bugs, assert that it's not already present - this'll stop any accidental overwriting of in-use page table
  // entries.
  mem_set_working_page_dir((unsigned long)table_phys_addr);
  table_addr = (unsigned long *)working_table_virtual_addr;
  encoded_entry = table_addr + page_dir_entry_idx;
  KL_TRC_DATA("Page dir Index", (unsigned long) page_dir_entry_idx);
  KL_TRC_DATA("table_addr", (unsigned long)table_addr);
  KL_TRC_DATA("encoded_entry addr", (unsigned long)encoded_entry);
  ASSERT(!PT_MARKED_PRESENT(*encoded_entry));

  new_entry.target_addr = phys_addr;
  new_entry.present = true;
  new_entry.writable = true;
  new_entry.user_mode = !is_kernel_allocation;
  new_entry.end_of_tree = true;
  new_entry.cache_type = (unsigned char)cache_mode;
  *encoded_entry = mem_encode_page_table_entry(new_entry);

  KL_TRC_DATA("Encoded entry", (unsigned long)*encoded_entry);

  KL_TRC_EXIT;
}

/// @brief Break the connection between a virtual memory address and its physical backing.
///
/// @param virt_addr The virtual memory address that will become unmapped.
void mem_unmap_virtual_page(unsigned long virt_addr)
{
  KL_TRC_ENTRY;

  unsigned long virt_addr_cpy = virt_addr;
  unsigned long pml4_entry_idx;
  unsigned long page_dir_ptr_entry_idx;
  unsigned long page_dir_entry_idx;
  unsigned long *table_addr = get_pml4_table_addr();
  unsigned long *encoded_entry;
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
  mem_set_working_page_dir((unsigned long)table_phys_addr);
  table_addr = (unsigned long *)working_table_virtual_addr;
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
  mem_set_working_page_dir((unsigned long)table_phys_addr);
  table_addr = (unsigned long *)working_table_virtual_addr;
  encoded_entry = table_addr + page_dir_entry_idx;
  *encoded_entry = 0;

  // We now need to flush this page table.
  mem_invalidate_page_table(virt_addr);

  KL_TRC_EXIT;
}

/// @brief Return the physical address of a 4kB "page" usable by the page table system by neatly carving up a 2MB page.
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
    next_4kb_page = (unsigned char *)mem_allocate_physical_pages(1);
  }

  ret_val = next_4kb_page;

  next_4kb_page = next_4kb_page + 4096;

  if (((unsigned long)next_4kb_page) % MEM_PAGE_SIZE == 0)
  {
    next_4kb_page = nullptr;
  }

  KL_TRC_EXIT;

  return ret_val;
}

/// @brief Set up a well-known virtual address to the given physical address.
///
/// @param phys_page_addr The physical page that needs mapping to working_table_va_entry_addr
void mem_set_working_page_dir(unsigned long phys_page_addr)
{
  KL_TRC_ENTRY;

  page_table_entry new_entry;
  unsigned long page_offset;

  KL_TRC_DATA("phys_page_addr", phys_page_addr);
  KL_TRC_DATA("working_table_va_entry_addr", (unsigned long)working_table_va_entry_addr);

  ASSERT(working_table_va_entry_addr != nullptr);
  ASSERT((phys_page_addr & 0x0FFF) == 0);

  page_offset = phys_page_addr & 0x1FFFFF;
  phys_page_addr = phys_page_addr & ~((unsigned long)0x1FFFFF);

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

  KL_TRC_DATA("working_table_va_entry_addr", (unsigned long)working_table_va_entry_addr);
  KL_TRC_DATA("*working_table_va_entry_addr", (unsigned long)(*working_table_va_entry_addr));

  *working_table_va_entry_addr = mem_encode_page_table_entry(new_entry);
  mem_invalidate_page_table(working_table_virtual_addr_base);
  working_table_virtual_addr = working_table_virtual_addr_base + page_offset;

  KL_TRC_DATA("page_offset", page_offset);
  KL_TRC_DATA("working_table_virtual_addr", (unsigned long)working_table_virtual_addr);

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
unsigned long mem_encode_page_table_entry(page_table_entry &pte)
{
  KL_TRC_ENTRY;

  unsigned char pat_value;

  unsigned long masked_addr = pte.target_addr & 0x0007FFFFFFFFF000;
  unsigned long result = masked_addr |
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
page_table_entry mem_decode_page_table_entry(unsigned long encoded)
{
  KL_TRC_ENTRY;

  page_table_entry decode;
  unsigned char pat_val;

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

  unsigned long virt_addr_cpy;
  unsigned long pml4_entry_idx;
  unsigned long page_dir_ptr_entry_idx;
  unsigned long page_dir_entry_idx;
  unsigned long *table_addr = get_pml4_table_addr(context);
  unsigned long *encoded_entry;
  void *table_phys_addr;
  unsigned long offset;
  bool return_addr_found = false;
  unsigned long phys_addr = 0;

  offset = ((unsigned long)virtual_addr) % MEM_PAGE_SIZE;
  virt_addr_cpy = ((unsigned long)virtual_addr) - offset;

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
    mem_set_working_page_dir((unsigned long)table_phys_addr);
    table_addr = (unsigned long *)working_table_virtual_addr;
    encoded_entry = table_addr + page_dir_ptr_entry_idx;
    if (PT_MARKED_PRESENT(*encoded_entry))
    {
      table_phys_addr = (void *)mem_x64_phys_addr_from_pte(*encoded_entry);

      // Having worked through all the page directories, grab the address out.
      mem_set_working_page_dir((unsigned long)table_phys_addr);
      table_addr = (unsigned long *)working_table_virtual_addr;
      encoded_entry = table_addr + page_dir_entry_idx;

      phys_addr = mem_x64_phys_addr_from_pte(*encoded_entry);
      phys_addr += offset;
      return_addr_found = true;
    }
  }

  KL_TRC_EXIT;

  return return_addr_found ? reinterpret_cast<void *>(phys_addr) : nullptr;
}

/// @brief Create the memory manager specific part of a process's information block.
///
/// Fill in a process memory information struct, which is provided back to the task manager, for it to live with all the
/// other process information.
///
/// @return A new, filled in, mem_process_info block. It is the callers responsibility to call
///         mem_task_destroy_task_entry when it is no longer needed.
mem_process_info *mem_task_create_task_entry()
{
  KL_TRC_ENTRY;

  mem_process_info *new_proc_info;
  process_x64_data *new_x64_proc_info;

  new_proc_info = new mem_process_info;
  KL_TRC_DATA("Created new memory manager information at", (unsigned long)new_proc_info);

  new_x64_proc_info = new process_x64_data;
  KL_TRC_DATA("Created new x64 information at", (unsigned long)new_x64_proc_info);

  mem_x64_pml4_allocate(*new_x64_proc_info);

  new_proc_info->arch_specific_data = (void *)new_x64_proc_info;

  KL_TRC_EXIT;
  return new_proc_info;
}

/// @brief Destroy a task's memory manager information block.
void mem_task_destroy_task_entry()
{
  panic("mem_task_destroy_task_entry not implemented");

  //Something to do with mem_x64_pml4_deallocate, eventually.
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
unsigned long *get_pml4_table_addr(task_process *context)
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Context", (unsigned long)context);

  task_thread *cur_thread = task_get_cur_thread();
  task_process *cur_process;
  mem_process_info *mem_info;
  process_x64_data *proc_data;
  unsigned long *table_addr;

  if (context != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Context provided, use appropriate PML4\n");
    mem_info = context->mem_info;
    ASSERT(mem_info != nullptr);
    proc_data = (process_x64_data *)mem_info->arch_specific_data;
    ASSERT(proc_data != nullptr);
    table_addr = (unsigned long *)proc_data->pml4_virt_addr;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No context provided, use current context\n");
    if(cur_thread != nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Provide process specific data\n");
      cur_process = cur_thread->parent_process;
      ASSERT(cur_process != nullptr);
      mem_info = cur_process->mem_info;
      ASSERT(mem_info != nullptr);
      proc_data = (process_x64_data *)mem_info->arch_specific_data;
      ASSERT(proc_data != nullptr);
      table_addr = (unsigned long *)proc_data->pml4_virt_addr;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No context provided, use current context\n");
      table_addr = (unsigned long *)task0_x64_entry.pml4_virt_addr;
    }
  }

  ASSERT(table_addr != nullptr);
  KL_TRC_DATA("Returning PML4 address", (unsigned long)table_addr);

  KL_TRC_EXIT;
  return table_addr;
}

/// @brief Convert an encoded PTE into the physical address backing it.
///
/// @param encoded An encoded PTE
///
/// @return The physical backing address.
unsigned long mem_x64_phys_addr_from_pte(unsigned long encoded)
{
  KL_TRC_ENTRY;

  page_table_entry decoded = mem_decode_page_table_entry(encoded);

  KL_TRC_EXIT;

  return decoded.target_addr;
}
