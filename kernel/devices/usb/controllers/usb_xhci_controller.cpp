/// @file
/// @brief USB xHCI controller implementation.

//#define ENABLE_TRACING

#include "devices/usb/controllers/usb_xhci_controller.h"
#include "processor/timing/timing.h"

#include "klib/klib.h"

using namespace usb::xhci;

namespace
{
  const uint64_t max_doorbell_size = 1024;
  const uint64_t max_runtime_regs_size = 32800;
}

controller::controller(pci_address address) :
  usb_gen_controller(address),
  _dev_status(DEV_STATUS::NOT_READY),
  capability_regs(nullptr),
  operational_regs(nullptr),
  command_ring(trb_command_ring(128)),
  runtime_regs_virt_addr(0),
  doorbell_regs(nullptr),
  interrupters(nullptr),
  scratchpad_virt_array_ptr(nullptr),
  scratchpad_phys_page_ptr_array(nullptr),
  num_scratchpad_page_ptrs(0)
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
    _dev_status = DEV_STATUS::FAILED;
  }
  else
  {
    // Set up the various rings and structures needed to run the controller.
    if (!prepare_control_structures())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to create control structures\n");
      _dev_status = DEV_STATUS::FAILED;
    }
    else
    {
      // Initialise interrupts.
      ASSERT(msi_configure(1, ints_granted));
      ASSERT(ints_granted == 1);

      // Start running!
      if (!msi_enable())
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to enable MSI - interrupts will not be received\n");
        _dev_status = DEV_STATUS::FAILED;
      }
      else
      {
        // Start the controller.
        operational_regs->usb_status.event_interrupt = 0;
        operational_regs->usb_command.interrupter_enable = 1;
        operational_regs->usb_command.run_stop = 1;

        // Do a quick trial.
        no_op_cmd_trb noct;
        noct.populate(false);
        ASSERT(command_ring.queue_trb(reinterpret_cast<basic_trb *>(&noct)));

        // ring doorbell.
        doorbell_regs[0] = 0;

        KL_TRC_TRACE(TRC_LVL::FLOW, "Short sleep\n");
        time_stall_process(1000000000);
        basic_trb trb;
        if (event_ring->dequeue_trb(&trb))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "TRB: ", trb.reserved_1, ", ", trb.reserved_2, "\n");
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "No TRBs\n");
        }
      }
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "USB status:", operational_regs->usb_status_raw, "\n");
  KL_TRC_TRACE(TRC_LVL::FLOW, "Interrupter 0 pending? ", interrupters[0].interrupt_management.pending, "\n");

  KL_TRC_EXIT;
}

controller::~controller()
{
  KL_TRC_ENTRY;

  operational_regs->usb_command.run_stop = 0;

  // Clean up the pages we mapped for scratchpads. Everything else should clean itself up.
  for (int i = 0; i < num_scratchpad_page_ptrs; i++)
  {
    mem_deallocate_physical_pages(reinterpret_cast<void *>(scratchpad_phys_page_ptr_array[i]), 1);
  }

  delete[] scratchpad_phys_page_ptr_array;
  delete[] scratchpad_virt_array_ptr;

  KL_TRC_EXIT;
}

const kl_string controller::device_name()
{
  return _dev_name;
}

DEV_STATUS controller::get_device_status()
{
  return _dev_status;
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

  // Confirm that all registers appear in the recently allocated page.
  virt_page_addr_num = reinterpret_cast<uint64_t>(virt_page_addr);
  ASSERT((reinterpret_cast<uint64_t>(capability_regs) + sizeof(caps_regs)) < (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(operational_regs) + sizeof(oper_regs)) < (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(runtime_regs_virt_addr) + max_runtime_regs_size) <
         (virt_page_addr_num + MEM_PAGE_SIZE));
  ASSERT((reinterpret_cast<uint64_t>(doorbell_regs) + max_doorbell_size) < (virt_page_addr_num + MEM_PAGE_SIZE));

  ASSERT(reinterpret_cast<uint64_t>(capability_regs) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(operational_regs) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(runtime_regs_virt_addr) > virt_page_addr_num);
  ASSERT(reinterpret_cast<uint64_t>(doorbell_regs) > virt_page_addr_num);

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
  //
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
    operational_regs->usb_command.run_stop = 0;
    time_stall_process(100000000);
    operational_regs->usb_command.hc_reset = 1;
    time_stall_process(1000000000);

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
  operational_regs->usb_command.run_stop = 0;


  // Initialise the Device Context Base Address Array, which is an array of pointers to device contexts. Since no
  // devices are enabled yet, all pointers are set to nullptr. The xHCI must be given the physical address, of course.
  device_ctxt_base_addr_array =
    std::make_unique<device_context *[]>(capability_regs->struct_params_1.max_device_slots + 1);
  kl_memset(device_ctxt_base_addr_array.get(),
            0,
            sizeof(device_context *) * (capability_regs->struct_params_1.max_device_slots + 1));

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
  KL_TRC_TRACE(TRC_LVL::FLOW, "Device context base array (v) ", device_ctxt_base_addr_array.get(), " (p) ", dcbaap_phys, "\n");

  // Set up the command ring.
  operational_regs->cmd_ring_cntrl = reinterpret_cast<uint64_t>(command_ring.get_phys_base_address());
  KL_TRC_TRACE(TRC_LVL::FLOW, "Command ring (p) ", operational_regs->cmd_ring_cntrl, "\n");

  // Also configure an event ring. The ring will add itself to the event ring tables.
  event_ring = std::make_unique<trb_event_ring>(1024, 0, this);

  // Configure Max Device Slots enabled.
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Max device slots: ", capability_regs->struct_params_1.max_device_slots, "\n");
  operational_regs->configure.max_device_slots_enabled = capability_regs->struct_params_1.max_device_slots;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;
  return result;
}

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

bool controller::handle_translated_interrupt_fast(unsigned char interrupt_offset, unsigned char raw_interrupt_num)
{
  KL_TRC_TRACE(TRC_LVL::FLOW, "xHCI fast interrupt\n");
  return false;
}
