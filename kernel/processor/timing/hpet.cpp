// Interface to the HPET.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/timing/timing.h"
#include "processor/timing/hpet.h"
extern "C"
{
#ifndef DOXYGEN_BUILD
#include "external/acpica/source/include/acpi.h"
#endif
}

hpet_hardware_cfg_block *hpet_config = nullptr;

void time_hpet_set_flag(uint64_t &hpet_reg, const uint64_t flag);
void time_hpet_clear_flag(uint64_t &hpet_reg, const uint64_t flag);
bool time_hpet_get_flag(uint64_t &hpet_reg, const uint64_t flag);

/// @brief Use ACPI to determine whether a HPET exists in this system.
///
/// @return True if a HPET exists, false otherwise.
bool time_hpet_exists()
{
  KL_TRC_ENTRY;

  acpi_table_hpet *hpet_table;
  ACPI_STATUS retval;
  bool hpet_exists;
  char table_name[] = "HPET";

  retval = AcpiGetTable(table_name, 0, (ACPI_TABLE_HEADER **)&hpet_table);
  hpet_exists = (retval == AE_OK);

  KL_TRC_EXIT;
  return hpet_exists;
}

/// @brief Initialize the HPET.
///
/// Initialize the HPET according to our requirements - the first channel for the multi-tasking interrupt, the second
/// for internal use. It is assumed that time_hpet_exists() would return true when this function is called, otherwise
/// it may panic.
void time_hpet_init()
{
  KL_TRC_ENTRY;

  acpi_table_hpet *hpet_table;
  ACPI_STATUS retval;
  char table_name[] = "HPET";
  uint64_t phys_addr;
  uint64_t offset;
  uint64_t virt_addr;
  uint64_t cap_flags;

  retval = AcpiGetTable(table_name, 0, (ACPI_TABLE_HEADER **)&hpet_table);
  ASSERT(retval == AE_OK);
  ASSERT(hpet_table->Address.Address != (uint64_t)nullptr);

  // Map the HPET configuration into the kernel's address space.
  phys_addr = hpet_table->Address.Address;
  offset = phys_addr % MEM_PAGE_SIZE;
  // Make sure the HPET config block fits within 1 page.
  ASSERT((offset + sizeof(hpet_hardware_cfg_block)) < MEM_PAGE_SIZE);
  phys_addr -= offset;

  virt_addr = (uint64_t)mem_allocate_virtual_range(1);
  mem_map_range((void *)phys_addr, (void *)virt_addr, 1);
  virt_addr += offset;

  // Perform some basic checks to make sure it is usable.
  hpet_config = (hpet_hardware_cfg_block *)virt_addr;
  cap_flags = hpet_config->gen_cap_and_id;
  ASSERT(HPET_REVISION(cap_flags) != 0);
  ASSERT(HPET_NUM_TIMERS(cap_flags) >= 2);
  ASSERT(HPET_PERIOD(cap_flags) <= max_period_fs);
  ASSERT(time_hpet_get_flag(cap_flags, hpet_hw_leg_rte_cap));

  KL_TRC_TRACE(TRC_LVL::EXTRA, "HPET general information:\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Revision: ", HPET_REVISION(cap_flags), "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Number of timers: ", HPET_NUM_TIMERS(cap_flags), "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Period in fs: ", HPET_PERIOD(cap_flags), "\n");

  // Stop the HPET while we configure it.
  time_hpet_clear_flag(hpet_config->gen_config, hpet_cfg_glbl_enable);

  // Keep legacy routing mode enabled, so that we can use different IRQs for timers 0 and 1.
  // (On QEMU at least, it will only route through one interrupt on the IOAPIC for all timers when not in legacy
  // replacement mode).
  time_hpet_set_flag(hpet_config->gen_config, hpet_cfg_leg_rte_map);

  // Configure timer 0 as a 100us periodic timer that calls IRQ 0. IRQ 0 is configured by task_install_task_switcher()
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Timer 0 config field before", hpet_config->timer_cfg[0].cfg_and_caps, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Timer 1 config field before", hpet_config->timer_cfg[1].cfg_and_caps, "\n");
  ASSERT(time_hpet_get_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_periodic_capable));
  ASSERT(time_hpet_get_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_64_bit_cap));
  //ASSERT((HPET_TMR_INT_RTE_CAP(hpet_config->timer_cfg[0].cfg_and_caps) & 1) != 0);

  //HPET_TMR_SET_INT_RTE(hpet_config->timer_cfg[0].cfg_and_caps, 0);
  time_hpet_clear_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_force_32_bit);
  time_hpet_set_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_periodic);
  time_hpet_clear_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_level_trig_int);

  // Set the period, and reset the main HPET counter to zero, so that it works properly!
  time_hpet_set_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_write_val);
  hpet_config->timer_cfg[0].comparator_val = time_hpet_compute_wait(time_task_mgr_int_period_ns);
  hpet_config->main_counter_val = 0;

  time_hpet_set_flag(hpet_config->timer_cfg[0].cfg_and_caps, hpet_tmr_enable);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Timer 0 config field after", hpet_config->timer_cfg[0].cfg_and_caps, "\n");

  // Configure timer 1 as a stopped 1-shot timer that calls IRQ 8.
  //ASSERT((HPET_TMR_INT_RTE_CAP(hpet_config->timer_cfg[1].cfg_and_caps) & 0x100) != 0);

  //HPET_TMR_SET_INT_RTE(hpet_config->timer_cfg[1].cfg_and_caps, 8);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_force_32_bit);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_periodic);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_level_trig_int);
  time_hpet_clear_flag(hpet_config->timer_cfg[1].cfg_and_caps, hpet_tmr_enable);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Timer 1 config field after", hpet_config->timer_cfg[1].cfg_and_caps, "\n");

  // Resume the HPET
  time_hpet_set_flag(hpet_config->gen_config, hpet_cfg_glbl_enable);

  KL_TRC_EXIT;
}

/// @brief Set the specified flag in a HPET register without affecting the rest.
///
/// @param hpet_reg The register to set the value in
///
/// @param flag The flag to set - ideally only one bit should be set, but the system will probably respond correctly if
///             multiple bits are set (no guarantees!)
void time_hpet_set_flag(uint64_t &hpet_reg, const uint64_t flag)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Flag being set", flag, "\n");

  uint64_t scratch;

  scratch = hpet_reg;
  scratch = scratch | flag;
  hpet_reg = scratch;

  KL_TRC_EXIT;
}

/// @brief Clear the specified flag in a HPET register without affecting the rest.
///
/// @param hpet_reg The register to set the value in
///
/// @param flag The flag to clear - ideally only one bit should be set, but the system will probably respond correctly
///             if multiple bits are set (no guarantees!)
void time_hpet_clear_flag(uint64_t &hpet_reg, const uint64_t flag)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Flag being cleared", flag, "\n");

  uint64_t scratch;

  scratch = hpet_reg;
  scratch = scratch & ~flag;
  hpet_reg = scratch;

  KL_TRC_EXIT;
}

/// @brief Determine whether the specified bit is set in a HPET register
///
/// @param hpet_reg The register to consider.
///
/// @param flag The flag to consider. Undefined behaviour results if anything other than exactly one bit is set.
///
/// @return True if the flag is set, false otherwise.
bool time_hpet_get_flag(uint64_t &hpet_reg, const uint64_t flag)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Flag being checked", flag, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Register value    ", hpet_reg, "\n");
  bool result = ((hpet_reg & flag) != 0);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result", result, "\n");

  KL_TRC_EXIT;

  return result;
}

/// @brief How long is a wait in HPET timer units?
///
/// Compute the value to be written to a HPET timer for the specified wait.
/// Makes two assumptions:
/// -1: That the wait period is small enough not to overflow the computed result (otherwise the return value is
///     incorrect)
/// -2: That the timer's counter starts at zero (the caller can simply add the current value if desired)
///
/// @param wait_in_ns How long is the desired wait, in nanoseconds
///
/// @return The number of HPET timer units corresponding to the desired wait.
uint64_t time_hpet_compute_wait(uint64_t wait_in_ns)
{
  KL_TRC_ENTRY;

  uint64_t wait_in_fs;
  uint64_t result;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Requested period (ns)", wait_in_ns, "\n");

  wait_in_fs = wait_in_ns * 1000000;
  result = wait_in_fs / HPET_PERIOD(hpet_config->gen_cap_and_id);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Number of cycles required", result, "\n");

  KL_TRC_EXIT;

  return result;
}

/// @brief Stall the process for the specified period.
///
/// Keeps running this process in a tight loop, but doesn't do anything to prevent the normal operation of the
/// scheduler!
///
/// @param wait_in_ns The number of nanoseconds to stall for.
void time_hpet_stall(uint64_t wait_in_ns)
{
  KL_TRC_ENTRY;

  uint64_t wait_in_cycles = time_hpet_compute_wait(wait_in_ns);
  uint64_t cur_count;
  uint64_t end_count;

  cur_count = hpet_config->main_counter_val;
  end_count = cur_count + wait_in_cycles;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Wait in ns", wait_in_ns, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Wait in cycles", wait_in_cycles, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Current cycle count", cur_count, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "End cycle count", end_count, "\n");

  while(end_count > cur_count)
  {
    cur_count = hpet_config->main_counter_val;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Actual end count", cur_count, "\n");

  KL_TRC_EXIT;
}

/// @brief Return the current value of the HPET counter.
///
/// @param output_in_ns Return the value in nanoseconds instead of HPET timer units?
///
/// @return The current value of the main HPET counter.
uint64_t time_hpet_cur_value(bool output_in_ns)
{
  uint64_t val;
  KL_TRC_ENTRY;

  val = hpet_config->main_counter_val;
  if (output_in_ns)
  {
    // Do two divisions by 1000 in case the Period is close to 1,000,000
    val = val * (HPET_PERIOD(hpet_config->gen_cap_and_id) / 1000);
    val = val / 1000;
  }

  KL_TRC_EXIT;

  return val;
}
