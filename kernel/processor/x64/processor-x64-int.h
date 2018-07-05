#pragma once

#include <stdint.h>

enum class PROC_IPI_MSGS;

// CPU Control
extern "C" void asm_proc_stop_this_proc();
extern "C" uint64_t asm_proc_read_msr(uint64_t msr);
extern "C" void asm_proc_write_msr(uint64_t msr, uint64_t value);
extern "C" uint64_t asm_proc_read_port(const uint64_t port_id, const uint8_t width);
extern "C" void asm_proc_write_port(const uint64_t port_id, const uint64_t value, const uint8_t width);
extern "C" void asm_proc_enable_fp_math();

// GDT Control
#define TSS_DESC_LEN 16
extern "C" void asm_proc_load_gdt();
extern uint8_t tss_gdt_entry[TSS_DESC_LEN];
void proc_init_tss();
void proc_load_tss(uint32_t proc_num);
extern "C" void asm_proc_load_tss(uint64_t gdt_offset);
void proc_recreate_gdt(uint32_t num_procs);

// Interrupt setup and handling
extern "C" void asm_proc_stop_interrupts();
extern "C" void asm_proc_start_interrupts();
extern "C" void asm_proc_def_interrupt_handler();
extern "C" void proc_def_interrupt_handler();
void proc_configure_idt();
void proc_configure_idt_entry(uint32_t interrupt_num, int32_t req_priv_lvl, void *fn_pointer, uint8_t ist_num);
extern "C" void asm_proc_install_idt();

extern "C" void *end_of_irq_ack_fn;

#define NUM_INTERRUPTS 256
#define IDT_ENTRY_LEN 16
extern uint8_t interrupt_descriptor_table[NUM_INTERRUPTS * IDT_ENTRY_LEN];

// Multi-processor control
void proc_mp_x64_signal_proc(uint32_t proc_id, PROC_IPI_MSGS msg);
extern "C" void proc_mp_x64_receive_signal_int();
extern "C" void proc_mp_ap_startup();

// Specialised interrupt handlers:
extern "C" void asm_proc_page_fault_handler();
extern "C" void proc_page_fault_handler(uint64_t fault_code, uint64_t fault_addr, uint64_t fault_instruction);
extern "C" void asm_task_switch_interrupt();

// IRQ handlers
extern "C" void asm_proc_handle_irq_0();
extern "C" void asm_proc_handle_irq_1();
extern "C" void asm_proc_handle_irq_2();
extern "C" void asm_proc_handle_irq_3();
extern "C" void asm_proc_handle_irq_4();
extern "C" void asm_proc_handle_irq_5();
extern "C" void asm_proc_handle_irq_6();
extern "C" void asm_proc_handle_irq_7();
extern "C" void asm_proc_handle_irq_8();
extern "C" void asm_proc_handle_irq_9();
extern "C" void asm_proc_handle_irq_10();
extern "C" void asm_proc_handle_irq_11();
extern "C" void asm_proc_handle_irq_12();
extern "C" void asm_proc_handle_irq_13();
extern "C" void asm_proc_handle_irq_14();
extern "C" void asm_proc_handle_irq_15();

// Exception handlers:
extern "C" void asm_proc_div_by_zero_fault_handler();  // 0
extern "C" void proc_div_by_zero_fault_handler();
extern "C" void asm_proc_debug_fault_handler(); // 1
extern "C" void proc_debug_fault_handler();
extern "C" void asm_proc_nmi_int_handler(); // 2
extern "C" void asm_proc_brkpt_trap_handler(); // 3
extern "C" void proc_brkpt_trap_handler();
extern "C" void asm_proc_overflow_trap_handler(); // 4
extern "C" void proc_overflow_trap_handler();
extern "C" void asm_proc_bound_range_fault_handler(); // 5
extern "C" void proc_bound_range_fault_handler();
extern "C" void asm_proc_invalid_opcode_fault_handler(); // 6
extern "C" void proc_invalid_opcode_fault_handler();
extern "C" void asm_proc_device_not_avail_fault_handler(); // 7
extern "C" void proc_device_not_avail_fault_handler();
extern "C" void asm_proc_double_fault_abort_handler(); // 8
extern "C" void proc_double_fault_abort_handler(uint64_t err_code, uint64_t rip);
extern "C" void asm_proc_invalid_tss_fault_handler(); // 10
extern "C" void proc_invalid_tss_fault_handler(uint64_t err_code, uint64_t rip);
extern "C" void asm_proc_seg_not_present_fault_handler(); // 11
extern "C" void proc_seg_not_present_fault_handler(uint64_t err_code, uint64_t rip);
extern "C" void asm_proc_ss_fault_handler(); // 12
extern "C" void proc_ss_fault_handler(uint64_t err_code, uint64_t rip);
extern "C" void asm_proc_gen_prot_fault_handler(); // 13
extern "C" void proc_gen_prot_fault_handler(uint64_t err_code, uint64_t rip);
extern "C" void asm_proc_fp_except_fault_handler(); // 16
extern "C" void proc_fp_except_fault_handler();
extern "C" void asm_proc_align_check_fault_handler(); // 17
extern "C" void proc_align_check_fault_handler(uint64_t err_code, uint64_t rip);
extern "C" void asm_proc_machine_check_abort_handler(); // 18
extern "C" void proc_machine_check_abort_handler();
extern "C" void asm_proc_simd_fpe_fault_handler(); // 19
extern "C" void proc_simd_fpe_fault_handler();
extern "C" void asm_proc_virt_except_fault_handler(); // 20
extern "C" void proc_virt_except_fault_handler();
extern "C" void asm_proc_security_fault_handler(); // 30
extern "C" void proc_security_fault_handler(uint64_t err_code, uint64_t rip);

// Helper functions
void *proc_x64_allocate_stack();

const uint64_t IRQ_BASE = 32;
