/// @file
/// @brief Functions internal to the memory manager.

#pragma once

#include <stdint.h>
#include "mem.h"

/// @brief The maximum number of physical pages supported by the kernel.
///
const uint64_t MEM_MAX_SUPPORTED_PAGES = 2048;

/// @brief The number of pages the kernel image requires in RAM.
///
/// This should be equivalent to the size of the kernel64.sys file + 1024kb to account for the fact that the kernel is
/// loaded at 1MB physical.
///
/// This number MUST match 'num_kernel_pages' in entry-x86.asm.
const uint16_t MEM_NUM_KERNEL_PAGES = 2;

void mem_init_gen_phys_sys(e820_pointer *e820_ptr);
void mem_gen_phys_pages_bitmap(e820_pointer *e820_ptr,
                               uint64_t *bitmap_loc,
                               uint64_t max_num_pages);


void mem_set_bitmap_page_bit(uint64_t page_addr, const bool ignore_checks = false);
void mem_clear_bitmap_page_bit(uint64_t page_addr, const bool ignore_checks = false);
bool mem_is_bitmap_page_bit_set(uint64_t page_addr);

void mem_map_init_counters();
void mem_map_virtual_page(uint64_t virt_addr,
                          uint64_t phys_addr,
                          task_process *context = nullptr,
                          MEM_CACHE_MODES cache_mode = MEM_WRITE_BACK);
void mem_unmap_virtual_page(uint64_t virt_addr, task_process *context, bool allow_phys_page_free);

void mem_vmm_init_proc_data(vmm_process_data &proc_data_ref);
void mem_vmm_free_proc_data(task_process *process);

// Allow the whole memory manager to access the data about task 0 (the kernel),
// to make it easier to feed it into other parts of the system later.
extern mem_process_info task0_entry;
