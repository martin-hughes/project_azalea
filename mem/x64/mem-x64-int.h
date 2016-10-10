// Contains x64-specific declarations.

#ifndef MEM_X64_INT_H_
#define MEM_X64_INT_H_

// Initial address of the PML4 paging address table.
extern unsigned long pml4_table;

const unsigned int PML4_LENGTH = 4096;

struct page_table_entry
{
  unsigned long target_addr;
  bool present;
  bool writable;
  bool user_mode;
  bool end_of_tree;
  unsigned char cache_type;
};

struct process_x64_data
{
  klib_list_item pml4_list_item;

  unsigned long pml4_phys_addr;
  unsigned long pml4_virt_addr;
};

unsigned long mem_encode_page_table_entry(page_table_entry &pte);
page_table_entry mem_decode_page_table_entry(unsigned long encoded);
void mem_set_working_page_dir(unsigned long phys_page_addr);
extern "C" void mem_invalidate_page_table(unsigned long virt_addr);
unsigned long mem_x64_phys_addr_from_pte(unsigned long encoded);

#define PT_MARKED_PRESENT(x) ((x) & 1)

void mem_x64_pml4_init_sys(process_x64_data &task0_data);
void mem_x64_pml4_allocate(process_x64_data &new_proc_data);
void mem_x64_pml4_deallocate(process_x64_data &proc_data);
void mem_x64_pml4_synchronize(void *updated_pml4_table);
unsigned long *get_pml4_table_addr(task_process *context = nullptr);

extern void *mem_x64_kernel_stack_ptr;

// x64 Cache control declarations

// Note the mapping between these and MEM_CACHE_MODES - the latter is meant to be platform independent, but at the
// moment (while only x64 is supported) they have a 1:1 mapping.
namespace MEM_X64_CACHE_TYPES
{
  const unsigned char UNCACHEABLE = 0;
  const unsigned char WRITE_COMBINING = 1;
  const unsigned char WRITE_THROUGH = 4;
  const unsigned char WRITE_PROTECTED = 5;
  const unsigned char WRITE_BACK = 6;
}

unsigned char mem_x64_pat_get_val(const unsigned char cache_type, bool first_half);
unsigned char mem_x64_pat_decode(const unsigned char pat_idx);

#endif
