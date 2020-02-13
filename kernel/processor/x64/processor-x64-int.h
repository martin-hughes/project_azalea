/// @file
/// @brief Functions internal to controlling x64 processors.

#pragma once

#include <stdint.h>

enum class PROC_IPI_MSGS;

// CPU Control

/// @brief Causes this CPU to halt.
///
extern "C" void asm_proc_stop_this_proc();

/// @brief Read an MSR on this CPU
///
/// @param msr The MSR number to read
///
/// @return The contents of the MSR.
extern "C" uint64_t asm_proc_read_msr(uint64_t msr);

/// @brief Write an MSR on this system
///
/// @param msr The MSR to write to
///
/// @param value The value to write to the MSR.
extern "C" void asm_proc_write_msr(uint64_t msr, uint64_t value);

/// @brief Read a CPU IO port
///
/// @param port_id The port to read from
///
/// @param width The number of bits to read (must be one of 8, 16, 32)
///
/// @return The value read from the port, zero-extended to 64-bits.
extern "C" uint64_t asm_proc_read_port(const uint64_t port_id, const uint8_t width);

/// @brief Write to a CPU IO port
///
/// @param port_id The port to write to
///
/// @param value The value to write to the port
///
/// @param width The number of bits to write (must be one of 8, 16, 32)
extern "C" void asm_proc_write_port(const uint64_t port_id, const uint64_t value, const uint8_t width);

/// @brief Enable floating-point math on this processor.
///
extern "C" void asm_proc_enable_fp_math();

// GDT Control
#define TSS_DESC_LEN 16 ///< Length of a single TSS descriptor
extern "C" void asm_proc_load_gdt(); ///< Load the system GDT onto this processor
extern uint8_t tss_gdt_entry[TSS_DESC_LEN]; ///< Storage for the GDT upon initial startup
void proc_init_tss();
void proc_load_tss(uint32_t proc_num);

/// @brief Load the TSS on this processor
///
/// @param gdt_offset The number of bytes from the start of the GDT for the TSS to load.
extern "C" void asm_proc_load_tss(uint64_t gdt_offset);
void proc_recreate_gdt(uint32_t num_procs);

// Interrupt setup and handling
extern "C" void asm_proc_stop_interrupts(); ///< @brief Stop interrupts on this processor.
extern "C" void asm_proc_start_interrupts(); ///< @brief Start interrupts on this processor.
void proc_configure_idt();
void proc_configure_idt_entry(uint32_t interrupt_num, int32_t req_priv_lvl, void *fn_pointer, uint8_t ist_num);
extern "C" void asm_proc_install_idt(); ///< @brief Install the system IDT on this processor.

extern "C" void *end_of_irq_ack_fn; ///< Function to call to acknowledge an IRQ.

#define NUM_INTERRUPTS 256 ///< Maximum supported interrupt number
#define IDT_ENTRY_LEN 16 ///< Length of an x64 IDT entry.
extern uint8_t interrupt_descriptor_table[NUM_INTERRUPTS * IDT_ENTRY_LEN];

// Multi-processor control
void proc_mp_x64_signal_proc(uint32_t proc_id, PROC_IPI_MSGS msg, bool must_complete);
extern "C" void proc_mp_x64_receive_signal_int();
extern "C" void proc_mp_ap_startup();
