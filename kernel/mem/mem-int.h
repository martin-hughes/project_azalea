#ifndef _MEM_INTERNAL_H
#define _MEM_INTERNAL_H

struct vmm_process_data;

void mem_init_gen_phys_sys(e820_pointer *e820_ptr);
void mem_gen_phys_pages_bitmap(e820_pointer *e820_ptr,
                               uint64_t *bitmap_loc,
                               uint64_t max_num_pages);


void mem_set_bitmap_page_bit(uint64_t page_addr, const bool ignore_checks);
void mem_clear_bitmap_page_bit(uint64_t page_addr);
bool mem_is_bitmap_page_bit_set(uint64_t page_addr);

void mem_map_virtual_page(uint64_t virt_addr,
                          uint64_t phys_addr,
                          task_process *context = nullptr,
                          MEM_CACHE_MODES cache_mode = MEM_WRITE_BACK);
void mem_unmap_virtual_page(uint64_t virt_addr);

void mem_vmm_init_proc_data(vmm_process_data &proc_data_ref);

// Allow the whole memory manager to access the data about task 0 (the kernel),
// to make it easier to feed it into other parts of the system later.
extern mem_process_info task0_entry;

#endif
