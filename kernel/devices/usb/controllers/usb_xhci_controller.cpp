/// @file
/// @brief USB xHCI controller implementation.

// Known deficiencies:
// - In handle_enable_slot_completion, we allocate contexts and so on with no code anywhere to delete them again.

//#define ENABLE_TRACING

#include "devices/usb/controllers/usb_xhci_controller.h"
#include "devices/usb/usb_xhci_device.h"
#include "processor/timing/timing.h"

#include "klib/klib.h"

using namespace usb::xhci;

namespace
{
  const uint64_t max_doorbell_size = 1024;
  const uint64_t max_runtime_regs_size = 32800;
}

/// @brief Standard constructor
///
/// @param address The PCI address of this xHCI controller.
controller::controller(pci_address address) :
  usb_gen_controller{address, "USB XHCI controller", "usb3"},
  capability_regs{nullptr},
  operational_regs{nullptr},
  command_ring{128},
  runtime_regs_virt_addr{0},
  doorbell_regs{nullptr},
  interrupters{nullptr},
  port_control_regs{nullptr},
  scratchpad_virt_array_ptr{nullptr},
  scratchpad_phys_page_ptr_array{nullptr},
  num_scratchpad_page_ptrs{0},
  root_ports{nullptr}
{
  bool hardware_ok;
  uint8_t ints_granted;

  // The steps inside these functions are based on the steps in the Intel xHCI Spec, v1.1, section 4.2.
  KL_TRC_ENTRY;

  // Set up the internal pointers to the various memory-mapped register blocks.
  initialize_registers(address);

  // Check the hardware is running OK.
  hardware_ok = initial_hardware_check();

  if (!hardware_ok)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Hardware failed\n");
    permanently_failed = true;
    set_device_status(DEV_STATUS::FAILED);
  }
  else
  {
    // Set up the various rings and structures needed to run the controller.
    if (!prepare_control_structures())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to create control structures\n");
      permanently_failed = true;
      set_device_status(DEV_STATUS::FAILED);
    }
    else
    {
      // Initialise interrupts.
      ASSERT(msi_configure(1, ints_granted));
      ASSERT(ints_granted == 1);
      set_device_status(DEV_STATUS::STOPPED);
    }
  }

  KL_TRC_EXIT;
}

controller::~controller()
{
  KL_TRC_ENTRY;

  this->controller_stop();

  // Clean up the pages we mapped for scratchpads. Everything else should clean itself up.
  for (int i = 0; i < num_scratchpad_page_ptrs; i++)
  {
    mem_deallocate_physical_pages(reinterpret_cast<void *>(scratchpad_phys_page_ptr_array[i]), 1);
  }

  delete[] scratchpad_phys_page_ptr_array;
  delete[] scratchpad_virt_array_ptr;

  KL_TRC_EXIT;
}

bool controller::start()
{
  bool result{true};

  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::STARTING);

  // Start running!
  if (permanently_failed || !msi_enable())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Permanently failed, or failed to enable MSI.\n");
    set_device_status(DEV_STATUS::FAILED);
  }
  else
  {
    // Start the controller.
    if(!controller_start())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Device failed to enter startup\n");
      set_device_status(DEV_STATUS::FAILED);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Started OK, kick ports\n");

      set_device_status(DEV_STATUS::OK);

      // Tell all ports that their status may have changed:
      for (uint32_t i = 0; i < capability_regs->struct_params_1.max_ports; i++)
      {
        if (root_ports[i].is_valid_port())
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Kick port ", i, "\n");
          root_ports[i].port_status_change_event();
        }
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool controller::stop()
{
  bool result{true};

  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::STOPPING);

  if (controller_stop())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Successful stop\n");
    set_device_status(DEV_STATUS::STOPPED);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to stop\n");
    set_device_status(DEV_STATUS::FAILED);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool controller::reset()
{
  bool result = true;

  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::RESET);

  if (operational_regs->usb_status.host_ctrlr_halted != 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Controller must stop before being reset\n");
    controller_stop();
  }

  if (!controller_reset())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to enter reset correctly\n");
    set_device_status(DEV_STATUS::FAILED);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device reset OK\n");
    set_device_status(DEV_STATUS::STOPPED);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief If the controller is halted, allow it to run.
///
/// Note that this is a separate action to IDevice::start() as the controller may have been paused for, for example, a
/// configuration change.
///
/// This function will pause for a short duration, if needed, to wait for the controller to start. If the controller
/// could not be started, the device status will be set to failed.
///
/// @return True if the controller was successfully started, false otherwise.
bool controller::controller_start()
{
  bool result{true};

  KL_TRC_ENTRY;

  if (operational_regs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Execute start.\n");
    operational_regs->usb_status.event_interrupt = 0;
    operational_regs->usb_command.interrupter_enable = 1;
    operational_regs->usb_command.run_stop = 1;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to start as not configured\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Set the controller to not be running.
///
/// This is subtly different to IDevice::stop() - this function may be called anytime we want to suspend operation of
/// the controller, for example to update its configuration, even when this is part of the normal operation.
/// IDevice::stop() means we want the device to stop semi-permanently.
///
/// This function will pause for a short duration, if needed, to wait for the controller to stop. If the controller
/// could not be stopped, the device status will be set to failed.
///
/// @return True If the controller was stopped successfully. False otherwise.
bool controller::controller_stop()
{
  bool result{true};
  uint64_t start_time;
  uint64_t end_time;

  KL_TRC_ENTRY;

  if (operational_regs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Stopping\n");
    operational_regs->usb_command.run_stop = 0;
    operational_regs->usb_command.interrupter_enable = 0;

    // The spec says the controller must become halted within 16ms.
    start_time = time_get_system_timer_count();
    end_time = start_time + time_get_system_timer_offset(16000000);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Wait for controller to halt\n");
    while (end_time > time_get_system_timer_count())
    {
      if (operational_regs->usb_status.host_ctrlr_halted == 1)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Controller stop observed\n");
        break;
      }
    }

    if (operational_regs->usb_status.host_ctrlr_halted == 1)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Controller stopped OK\n");
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Controller failed to stop\n");
      result = false;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Controller not configured, unable to stop\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief If the controller is stopped, attempt a reset.
///
/// This is independent of IDevice::reset() as there are legitimate reasons to get the controller chip to reset itself
/// without updating the state of this object - for example, resetting the chip during initialisation of this driver.
///
/// This function will pause for a short duration, if needed, to wait for the controller to reset. If the controller
/// could not be reset, the device status will be set to failed.
///
/// @return True if the controller was successfully reset, false otherwise.
bool controller::controller_reset()
{
  bool result{true};
  uint64_t start_time;
  uint64_t end_time;

  KL_TRC_ENTRY;

  ASSERT(operational_regs);
  if (operational_regs->usb_status.host_ctrlr_halted == 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "OK to reset\n");
    operational_regs->usb_command.hc_reset = 1;

    // Wait up to 1 second for the controller to be ready again.
    start_time = time_get_system_timer_count();
    end_time = start_time + time_get_system_timer_offset(1000000000);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Wait for controller\n");
    while (end_time > time_get_system_timer_count())
    {
      if ((operational_regs->usb_status.controller_not_ready == 0) && (operational_regs->usb_command.hc_reset == 0))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Controller ready\n");
        break;
      }
    }

    if ((operational_regs->usb_status.controller_not_ready != 0) || (operational_regs->usb_command.hc_reset != 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Controller failed to reset\n");
      set_device_status(DEV_STATUS::FAILED);
      result = false;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Can only reset while stopped\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

// Initialization helpers
// ----------------------

/// @brief Simply initialize all the pointers to the various memory mapped register blocks an xHCI controller has.
///
/// @param address The PCI address of this controller.
void controller::initialize_registers(pci_address address)
{
  uint32_t base_addr_low;
  uint32_t base_addr_high;
  uint64_t base_addr_num;
  uint64_t base_addr_offset;
  uint64_t base_addr_page;
  uint64_t cap_offset;
  void *virt_page_addr;

  uint64_t virt_page_addr_num;

  KL_TRC_ENTRY;

  // Retrieve the physical address of the xHCI capabilities registers.
  base_addr_low = pci_read_raw_reg(address, PCI_REGS::BAR_0) & 0xFFFFFFF0;
  base_addr_high = pci_read_raw_reg(address, PCI_REGS::BAR_1);
  base_addr_num = base_addr_high;
  base_addr_num <<= 32;
  base_addr_num |= base_addr_low;

  // Map that into virtual space and calculate the register pointer addresses.
  klib_mem_split_addr(base_addr_num, base_addr_page, base_addr_offset);
  virt_page_addr = mem_allocate_virtual_range(1);
  mem_map_range(reinterpret_cast<void *>(base_addr_page), virt_page_addr, 1, nullptr, MEM_UNCACHEABLE);

  capability_regs = reinterpret_cast<caps_regs *>(reinterpret_cast<uint64_t>(virt_page_addr) + base_addr_offset);
  operational_regs = reinterpret_cast<oper_regs *>(reinterpret_cast<uint64_t>(capability_regs) +
                                                   capability_regs->caps_length);

  runtime_regs_virt_addr = reinterpret_cast<uint64_t>(capability_regs) + capability_regs->runtime_regs_offset;

  doorbell_regs = reinterpret_cast<uint32_t *>(reinterpret_cast<uint64_t>(capability_regs) +
                                               capability_regs->doorbell_offset);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Doorbell (v) ", doorbell_regs, "\n");

  interrupters = reinterpret_cast<interrupter_regs *>(runtime_regs_virt_addr + 0x20);

  port_control_regs = reinterpret_cast<port_regs *>(reinterpret_cast<uint64_t>(operational_regs) + 1024);

  cap_offset = capability_regs->capability_params_1.extended_caps_ptr << 2;
  extended_caps = reinterpret_cast<extended_cap_hdr *>(reinterpret_cast<uint64_t>(capability_regs) + cap_offset);

  // Confirm that all registers appear in the recently allocated page.
  virt_page_addr_num = reinterpret_cast<uint64_t>(virt_page_addr);
  ASSERT((reinterpret_cast<uint64_t>(capability_regs) + sizeof(caps_regs)) < (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(operational_regs) + sizeof(oper_regs)) < (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(runtime_regs_virt_addr) + max_runtime_regs_size) <
         (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(doorbell_regs) + max_doorbell_size) < (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(port_control_regs) + max_doorbell_size) < (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(extended_caps) + max_doorbell_size) < (virt_page_addr_num + MEM_PAGE_SIZE));

  ASSERT(reinterpret_cast<uint64_t>(capability_regs) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(operational_regs) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(runtime_regs_virt_addr) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(doorbell_regs) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(port_control_regs) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(extended_caps) > virt_page_addr_num);

  KL_TRC_EXIT;
}

/// @brief Check the hardware is running and supported.
///
/// For the time being we only support using MSI interrupts, so they must be supported by the controller.
///
/// @return true if the device is started within a reasonable time and supports using MSI. False otherwise.
bool controller::initial_hardware_check()
{
  uint64_t start_time;
  uint64_t end_time;
  bool ready;

  KL_TRC_ENTRY;

  // Wait up to 1 second for the controller to be ready. Once the controller is ready, the steps that follow are based
  // on the xHCI spec, section 4.2.
  start_time = time_get_system_timer_count();
  end_time = start_time + time_get_system_timer_offset(1000000000);

  KL_TRC_TRACE(TRC_LVL::FLOW, "Wait for controller\n");
  while (end_time > time_get_system_timer_count())
  {
    if (operational_regs->usb_status.controller_not_ready == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Controller ready\n");
      break;
    }
  }

  if (operational_regs->usb_status.controller_not_ready != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Hardware didn't start in time, assume failed\n");
    ready = false;
  }
  else
  {
    // For an easier ride writing the driver, assume xHCI controllers must support MSI.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Hardware started, check compatibility\n");

    // Make sure the controller is stopped before fiddling with it.
    this->controller_stop();
    this->controller_reset();

    if (caps.msi.supported == false)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "MSI not supported!\n");
      ready = false;
    }
    else
    {
      ready = true;
    }
  }

  KL_TRC_EXIT;
  return ready;
}

/// @brief Prepare the command structures for the controller.
///
/// @return true if all control structures were created successfully, false otherwise.
bool controller::prepare_control_structures()
{
  void *dcbaap_phys;
  uint16_t num_scratchpads;
  uint64_t scratchpad_array_phys_ptr;
  bool result = true;

  KL_TRC_ENTRY;

  // Set controller to stopped.
  this->controller_stop();

  // Initialise the Device Context Base Address Array, which is an array of pointers to device contexts. Since no
  // devices are enabled yet, all pointers are set to nullptr. The xHCI must be given the physical address, of course.
  device_ctxt_base_addr_array =
    std::make_unique<device_context *[]>(capability_regs->struct_params_1.max_device_slots + 1);
  memset(device_ctxt_base_addr_array.get(),
         0,
         sizeof(device_context *) * (capability_regs->struct_params_1.max_device_slots + 1));

  // At the same time, create an array to allow us to correlate slot numbers to device cores.
  slot_to_device_obj_map =
    std::make_unique<std::shared_ptr<device_core>[]>(capability_regs->struct_params_1.max_device_slots + 1);
  memset(slot_to_device_obj_map.get(),
         0,
         sizeof(device_core *) * (capability_regs->struct_params_1.max_device_slots + 1));

  // If the controller needs it, add some scratchpad space via the DCBAA.
  num_scratchpads = (capability_regs->struct_params_2.max_scratchpad_bufs_hi << 5) &
    capability_regs->struct_params_2.max_scratchpad_bufs_lo;
  if (num_scratchpads != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Request scratchpad buffers\n");
    scratchpad_array_phys_ptr = generate_scratchpad_array(num_scratchpads);

    if (scratchpad_array_phys_ptr == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to create scratchpad array\n");
      result = false;
    }
    else
    {
      device_ctxt_base_addr_array[0] = reinterpret_cast<device_context *>(scratchpad_array_phys_ptr);
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Operational regs (v) ", operational_regs, "\n");

  dcbaap_phys = mem_get_phys_addr(device_ctxt_base_addr_array.get());
  ASSERT(dcbaap_phys != nullptr);
  // For now, assert in the lowest 4GB of RAM. Since we officially only support 1GB, this ought not be a problem!
  ASSERT(reinterpret_cast<uint64_t>(dcbaap_phys) < 0x100000000);
  operational_regs->dev_cxt_base_addr_ptr = reinterpret_cast<uint64_t>(dcbaap_phys);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Device context base array (v) ", device_ctxt_base_addr_array.get(), " (p) ",
                              dcbaap_phys, "\n");

  // Set up the command ring.
  operational_regs->cmd_ring_cntrl = reinterpret_cast<uint64_t>(command_ring.get_phys_base_address());
  KL_TRC_TRACE(TRC_LVL::FLOW, "Command ring (p) ", operational_regs->cmd_ring_cntrl, "\n");

  // Also configure an event ring. The ring will add itself to the event ring tables.
  event_ring = std::make_unique<trb_event_ring>(1024, 0, this);

  // Configure Max Device Slots enabled.
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Max device slots: ", capability_regs->struct_params_1.max_device_slots, "\n");
  operational_regs->configure.max_device_slots_enabled = capability_regs->struct_params_1.max_device_slots;

  // Create structures to track the root hub ports. These will be populated once the types of the ports are given by
  // the extended capabilities structures.
  root_ports = std::unique_ptr<root_port[]>(new root_port[capability_regs->struct_params_1.max_ports + 1]);

  // Scan all extended capabilities. This will have the side-effect of fully populating the port structures created
  // above.
  result = examine_extended_caps() && result;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

/// @brief Generate the required number of scratchpads for the xHCI.
///
/// @param num_scratchpads The number of scratchpads required.
///
/// @return The physical address of the scratchpad pointer array.
uint64_t controller::generate_scratchpad_array(uint16_t num_scratchpads)
{
  uint64_t array_phys_ptr;
  uint64_t total_scratchpad_size;
  uint32_t actual_page_size; // The size of a page as far as the xHCI is concerned.
  uint32_t num_pages; // The number of physical pages required.
  uint64_t scratchpad_size_allocated;
  uint32_t cur_page;
  uint32_t cur_offset;
  int i;

  KL_TRC_ENTRY;

  if ((num_scratchpads > 1024) || (num_scratchpads == 0))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameter\n");
    array_phys_ptr = 0;
  }
  else
  {
    actual_page_size = 1 << (operational_regs->page_size + 12);
    if (actual_page_size > MEM_PAGE_SIZE)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to allocate contiguous blocks greater than 2MB.\n");
      array_phys_ptr = 0;
    }
    else
    {
      total_scratchpad_size = num_scratchpads * operational_regs->page_size;

      // Calculate how many physical pages we need to cover the scratchpad requirements and allocate those pages.
      num_pages = total_scratchpad_size / MEM_PAGE_SIZE;
      if ((total_scratchpad_size % MEM_PAGE_SIZE) != 0)
      {
        num_pages++;
      }
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Allocating ", num_pages, " physical pages\n");

      scratchpad_phys_page_ptr_array = new uint64_t[num_pages];
      num_scratchpad_page_ptrs = num_pages;

      for (int i = 0; i < num_pages; i++)
      {
        scratchpad_phys_page_ptr_array[i] = reinterpret_cast<uint64_t>(mem_allocate_physical_pages(1));
      }

      // Now, fill in an array of pointers to pages that the xHCI will understand. Fortunately, since pages have to be
      // powers of two in size, they will never overlap a physical page boundary.
      uint64_t *page_ptr_array = new uint64_t[num_scratchpads];
      array_phys_ptr = reinterpret_cast<uint64_t>(mem_get_phys_addr(reinterpret_cast<void *>(page_ptr_array)));
      scratchpad_virt_array_ptr = page_ptr_array;

      for (scratchpad_size_allocated = 0, i = 0;
           i < num_scratchpads;
           i++, scratchpad_size_allocated += actual_page_size)
      {
        cur_page = scratchpad_size_allocated / MEM_PAGE_SIZE;
        cur_offset = scratchpad_size_allocated % MEM_PAGE_SIZE;
        page_ptr_array[i] = scratchpad_phys_page_ptr_array[cur_page] + cur_offset;
        KL_TRC_TRACE(TRC_LVL::FLOW, "Setting scratchpad ", i, " to ", page_ptr_array[i], "\n");
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", array_phys_ptr, "\n");
  KL_TRC_EXIT;

  return array_phys_ptr;
}

//----------------------
// Interrupt handlers.
//----------------------

bool controller::handle_translated_interrupt_fast(uint8_t interrupt_offset, uint8_t raw_interrupt_num)
{
  KL_TRC_TRACE(TRC_LVL::FLOW, "xHCI fast interrupt\n");
  return true;
}

void controller::handle_translated_interrupt_slow(uint8_t interrupt_offset, uint8_t raw_interrupt_num)
{
  template_trb cur_trb;
  bool received_trb;

  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "xHCI Slow interrupt for # ", interrupt_offset, " (raw :", raw_interrupt_num, ")\n");

  received_trb = event_ring->dequeue_trb(&cur_trb);
  while (received_trb)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Received TRB: ", cur_trb.trb_type, "\n");
    dispatch_event_trb(cur_trb);

    received_trb = event_ring->dequeue_trb(&cur_trb);
  }

  KL_TRC_EXIT;
}

/// @brief Having received a TRB in an event ring, deal with it.
///
/// @param trb The TRB to handle.
void controller::dispatch_event_trb(template_trb &trb)
{
  KL_TRC_ENTRY;

  switch (trb.trb_type)
  {
  case TRB_TYPES::PORT_STS_CHANGE_EVENT:
    // Handle port status change event.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Port status change event\n");
    handle_port_status_change_event(*(reinterpret_cast<port_status_change_event_trb *>(&trb)));
    break;

  case TRB_TYPES::COMMAND_COMPLETE_EVENT:
    // Handle command completion event.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Command completion event\n");
    handle_command_completion(*(reinterpret_cast<command_completion_event_trb *>(&trb)));
    break;

  case TRB_TYPES::TRANSFER_EVENT:
    // Handle transfer event.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer event\n");
    handle_transfer_event(*(reinterpret_cast<transfer_event_trb *>(&trb)));
    break;

  case TRB_TYPES::BANDWIDTH_REQUEST_EVENT:
  case TRB_TYPES::DOORBELL_EVENT:
  case TRB_TYPES::HOST_CONTROLLER_EVENT:
  case TRB_TYPES::DEVICE_NOTFN_EVENT:
  case TRB_TYPES::MFINDEX_WRAP_EVENT:
    // Other valid, but currently unhandled TRB types.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unhandled valid TRB of type: ", trb.trb_type, "\n");
    break;

  default:
    // We don't recognise this TRB type.
    KL_TRC_TRACE(TRC_LVL::ERROR, "Invalid event TRB received\n");
    break;
  }

  KL_TRC_EXIT;
}

//-----------------------------------------------------------
// Event Handlers. These all run in the interrupt slow path.
//-----------------------------------------------------------

/// @brief Handles a Port Status Change Event.
///
/// @param trb A PSC event TRB generated by the controller.
void controller::handle_port_status_change_event(port_status_change_event_trb &trb)
{
  uint32_t port_id;

  KL_TRC_ENTRY;

  port_id = trb.port_id - 1;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Port changed: ", trb.port_id, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Port CCS: ", port_control_regs[port_id].status_ctrl.current_connect_status);
  KL_TRC_TRACE(TRC_LVL::FLOW, ", CCS change: ", port_control_regs[port_id].status_ctrl.connect_status_change, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Port link status:", port_control_regs[port_id].status_ctrl.port_link_status, "\n");

  root_ports[trb.port_id].port_status_change_event();

  KL_TRC_EXIT;
}

/// @brief Handles a Command Completion Event
///
/// @param trb A Command Completion event TRB generated by the controller.
void controller::handle_command_completion(command_completion_event_trb &trb)
{
  xhci_command_data *cmd_data;

  KL_TRC_ENTRY;

  cmd_data = command_ring.retrieve_command(trb);
  if (cmd_data != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieved command data\n");

    switch (cmd_data->generated_trb.trb_type)
    {
    case TRB_TYPES::ENABLE_SLOT_CMD:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Enable slot command completed\n");
      handle_enable_slot_completion(trb, cmd_data->requesting_device);
      break;

    case TRB_TYPES::ADDRESS_DEVICE_CMD:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Address device command completed\n");
      handle_address_device_completion(trb, cmd_data->requesting_device);
      break;

    // These commands are generated by devices, not internally, so simply indicate to them that their command has
    // completed.
    case TRB_TYPES::EVAL_CONTEXT_CMD:
    case TRB_TYPES::CONFIG_ENDPOINT_CMD:
      if (cmd_data->response_item != nullptr)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Return response data\n");
        cmd_data->response_item->trb = trb;

        if (cmd_data->requesting_device)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Send message to requesting device\n");
          std::unique_ptr<command_complete_msg> msg =
            std::make_unique<command_complete_msg>(static_cast<uint8_t>(cmd_data->generated_trb.trb_type),
                                                   static_cast<uint8_t>(trb.completion_code));
          work::queue_message(cmd_data->requesting_device, std::move(msg));
        }
      }
      break;

    // These commands are not currently expected to be seen.
    case TRB_TYPES::DISABLE_SLOT_CMD:
    case TRB_TYPES::RESET_ENDPOINT_CMD:
    case TRB_TYPES::STOP_ENDPOINT_CMD:
    case TRB_TYPES::SET_TR_DEQUEUE_PTR_CMD:
    case TRB_TYPES::RESET_DEVICE_CMD:
    case TRB_TYPES::FORCE_EVENT_CMD:
    case TRB_TYPES::NEGOTIATE_BANDWIDTH_CMD:
    case TRB_TYPES::SET_LATENCY_TOL_CMD:
    case TRB_TYPES::GET_PORT_BANDWIDTH_CMD:
    case TRB_TYPES::FORCE_HEADER_CMD:
    case TRB_TYPES::NO_OP_CMD:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unsupported TRB type.\n");
      break;
    }

    delete cmd_data;
    cmd_data = nullptr;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to retrieve command data - skip.\n");
  }

  KL_TRC_EXIT;
}

/// @brief Handles the response to an enable slot command
///
/// @param trb The command completion event TRB corresponding to this command.
///
/// @param requesting_dev The device that requested the slot.
void controller::handle_enable_slot_completion(command_completion_event_trb &trb,
                                               std::shared_ptr<device_core> requesting_dev)
{
  KL_TRC_ENTRY;

  uint32_t new_slot = trb.slot_id;

  // Fill in a slot structure, then inform the device it has an enabled slot.
  if (trb.completion_code == C_CODES::SUCCESS)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Slot enabled successfully - slot ", new_slot, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "Raw: ", ((template_trb *)&trb)->reserved_2, "\n");

    device_context *out_context = new device_context;
    memset(out_context, 0, sizeof(device_context));

    device_ctxt_base_addr_array[new_slot] = reinterpret_cast<device_context *>(mem_get_phys_addr(out_context));
    ASSERT(slot_to_device_obj_map[new_slot] == nullptr);
    slot_to_device_obj_map[new_slot] = requesting_dev;

    requesting_dev->handle_slot_enabled(new_slot, out_context);
  }
  else
  {
    INCOMPLETE_CODE("Failed to enable slot");
  }

  KL_TRC_EXIT;
}

/// @brief Handles the response to an Address Device command
///
/// @param trb The command completion event TRB corresponding to this command.
///
/// @param requesting_dev The device that requested addressing.
void controller::handle_address_device_completion(command_completion_event_trb &trb,
                                                  std::shared_ptr<device_core> requesting_dev)
{
  KL_TRC_ENTRY;

  if (trb.completion_code == C_CODES::SUCCESS)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Address device successful\n");
    requesting_dev->handle_addressed();
  }
  else
  {
    INCOMPLETE_CODE("Failed to address device");
  }

  KL_TRC_EXIT;
}

/// @brief Handle a controller-generated Transfer Event
///
/// @param trb The Transfer Event TRB generated by the controller.
void controller::handle_transfer_event(transfer_event_trb &trb)
{
  std::shared_ptr<device_core> core;
  KL_TRC_ENTRY;

  core = slot_to_device_obj_map[trb.slot_id];

  KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer complete for device in slot ", trb.slot_id, " - ", core.get(), "\n");

  ASSERT(core != nullptr);
  core->handle_transfer_event(trb);

  KL_TRC_EXIT;
}

//------------------------------------
// Driver generated command requests.
//------------------------------------

/// @brief Requests a slot for this device.
///
/// Corresponds to generating an Enable Slot command for this device.
///
/// @param req_dev The device requesting a slot.
void controller::request_slot(std::shared_ptr<device_core> req_dev)
{
  enable_slot_cmd_trb cmd_trb;

  // This object is deleted by the command completion handler.
  xhci_command_data *new_cmd = new xhci_command_data;

  KL_TRC_ENTRY;

  cmd_trb.populate(false, root_ports[req_dev->get_port_num()].get_required_slot_type());

  copy_trb(&new_cmd->generated_trb, reinterpret_cast<template_trb *>(&cmd_trb));
  new_cmd->requesting_device = req_dev;

  command_ring.queue_command(new_cmd);
  doorbell_regs[0] = 0;

  KL_TRC_EXIT;
}

/// @brief Triggers the xHCI to address this device.
///
/// Corresponds to generating an Address Device command for this device.
///
/// @param req_dev The device requesting addressing.
///
/// @param input_ctxt_phys_addr The physical address of the input context of this device.
///
/// @param slot_id The slot ID for the device being addressed.
void controller::address_device(std::shared_ptr<device_core> req_dev, uint64_t input_ctxt_phys_addr, uint8_t slot_id)
{
  address_device_cmd_trb cmd_trb;
  xhci_command_data *new_cmd = new xhci_command_data; // Deleted by the command completion handler.

  KL_TRC_ENTRY;

  cmd_trb.populate(true, false, input_ctxt_phys_addr, false, slot_id);
  copy_trb(&new_cmd->generated_trb, reinterpret_cast<template_trb *>(&cmd_trb));
  new_cmd->requesting_device = req_dev;

  command_ring.queue_command(new_cmd);
  doorbell_regs[0] = 0;

  KL_TRC_EXIT;
}

/// @brief Handles a generic driver-initiated command for a device.
///
/// This command WILL block until a response to the command is received.
///
/// @param trb Pointer to the command TRB to send.
///
/// @param req_dev The requesting device.
///
/// @return True if the command completed successfully, false otherwise.
bool controller::generic_device_command(template_trb *trb, std::shared_ptr<device_core> req_dev)
{
  xhci_command_data *new_cmd = new xhci_command_data; // Deleted by the command completion handler.
  std::shared_ptr<command_response> response = std::make_shared<command_response>();
  bool result = true;

  KL_TRC_ENTRY;

  copy_trb(&new_cmd->generated_trb, trb);
  new_cmd->requesting_device = req_dev;
  new_cmd->response_item = response;

  result = command_ring.queue_command(new_cmd);
  // After here, new_cmd is no longer valid - it is deleted once the command completion handler is called.
  new_cmd = nullptr;
  doorbell_regs[0] = 0;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Triggers the xHCI to update the contexts of this device.
///
/// Corresponds to generating an Evaluate Context command for this device.
///
/// @param req_dev The device requesting addressing.
///
/// @param input_ctxt_phys_addr The physical address of the input context of this device.
///
/// @param slot_id The slot ID for the device being addressed.
///
/// @return True if the Evaluate Context command completed successfully, false otherwise.
bool controller::evaluate_context(std::shared_ptr<device_core> req_dev, uint64_t input_ctxt_phys_addr, uint8_t slot_id)
{
  bool result = true;
  evaluate_context_cmd_trb cmd_trb;

  KL_TRC_ENTRY;

  cmd_trb.populate(false, false, input_ctxt_phys_addr, false, slot_id);

  result = generic_device_command(reinterpret_cast<template_trb *>(&cmd_trb), req_dev);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Triggers the xHCI to configure the endpoints of this device.
///
/// Corresponds to generating an Configure Endpoint command for this device.
///
/// @param req_dev The device requesting addressing.
///
/// @param input_ctxt_phys_addr The physical address of the input context of this device. It's assumed the caller will
///                             set up the context as desired.
///
/// @param slot_id The slot ID for the device being addressed.
///
/// @return True if the endpoint configuration was successful, false otherwise.
bool controller::configure_endpoints(std::shared_ptr<device_core> req_dev,
                                     uint64_t input_ctxt_phys_addr,
                                     uint8_t slot_id)
{
  bool result = true;
  configure_endpoint_cmd_trb cmd_trb;

  KL_TRC_ENTRY;

  cmd_trb.input_context_ptr_phys = input_ctxt_phys_addr;
  cmd_trb.slot_id = slot_id;

  result = generic_device_command(reinterpret_cast<template_trb *>(&cmd_trb), req_dev);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Ring the specified doorbell with the given code.
///
/// @param doorbell_num The number of the doorbell to ring.
///
/// @param endpoint_code The endpoint ringing the doorbell. One of EP_DOORBELL_CODES is suggested, but not required.
///
/// @param stream_id Optional, default 0. If the endpoint implements streams, the ID of the stream the doorbell ring is
///                  targeting.
void controller::ring_doorbell(uint8_t doorbell_num, uint8_t endpoint_code, uint16_t stream_id)
{
  KL_TRC_ENTRY;

  uint32_t doorbell_code = stream_id;
  doorbell_code <<= 16;
  doorbell_code |= endpoint_code;

  doorbell_regs[doorbell_num] = doorbell_code;

  KL_TRC_EXIT;
}

command_complete_msg::command_complete_msg(uint8_t cmd, uint8_t code) :
  msg::root_msg{SM_XHCI_CMD_COMPLETE},
  generated_command{cmd},
  completion_code{code}
{

}
