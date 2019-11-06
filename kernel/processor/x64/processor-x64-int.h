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
