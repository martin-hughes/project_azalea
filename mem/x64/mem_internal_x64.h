// Contains x64-specific declarations.

#ifndef MEM_INTERNAL_X64_H_
#define MEM_INTERNAL_X64_H_

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
};

struct process_x64_data
{
  klib_list_item pml4_list_item;

  unsigned long pml4_phys_addr;
  unsigned long pml4_virt_addr;
};

#ifndef NULL
#define NULL ((void *)0)
#endif

unsigned long mem_encode_page_table_entry(page_table_entry &pte);
page_table_entry mem_decode_page_table_entry(unsigned long encoded);
void mem_set_working_page_dir(unsigned long phys_page_addr);
extern "C" void mem_invalidate_page_table(unsigned long virt_addr);
void *mem_get_phys_addr(void *virtual_addr);

#define PHYS_ADDR_FROM_PTE(x) ((x) & (0x0003FFFFFFFFF000))
#define PT_MARKED_PRESENT(x) ((x) & 1)

void mem_x64_pml4_init_sys(process_x64_data &task0_data);
void mem_x64_pml4_allocate(process_x64_data &new_proc_data);
void mem_x64_pml4_deallocate(process_x64_data &proc_data);
void mem_x64_pml4_synchronize(void *updated_pml4_table);
unsigned long *get_pml4_table_addr(task_process *context = (task_process *)NULL);

extern void *mem_x64_kernel_stack_ptr;

#endif
