/// @file
/// @brief Generic virtio device functionality.
//
// Known defects:
// - The possibility that the device configuration might change is not handled at all.

//#define ENABLE_TRACING

#include <atomic>

#include "virtio.h"
#include "../devices/pci/generic_device/pci_generic_device.h"
#include "processor.h"


namespace virtio
{

/// @brief Default constructor
///
/// @param address PCI address of this device.
///
/// @param human_name Human-readable name for this device.
///
/// @param dev_name Machine name for this device.
generic_device::generic_device(pci_address address, std::string human_name, std::string dev_name) :
  pci_generic_device(address, human_name, dev_name)
{
  KL_TRC_ENTRY;

  KL_TRC_ENABLE_OUTPUT();

  if (!read_pci_config())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read PCI config\n");
    set_device_status(OPER_STATUS::FAILED);
  }

  // Generic setup steps:
  // 1 - Reset device.
  common_cfg->device_status = 0;

  // 2 - Set ACKNOWLEDGE and DRIVER bits.
  common_cfg->device_status = 3;

  // Assume that all queues are going to be used and at maximum size.
  for (uint16_t i = 0; i < common_cfg->num_queues; i++)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Construct queue ", i, "\n");
    common_cfg->queue_select = i;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    queues.emplace_back(this, common_cfg->queue_size, i);

    common_cfg->queue_desc = queues[i].descriptor_phys;
    common_cfg->queue_device = queues[i].used_ring_phys;
    common_cfg->queue_driver = queues[i].avail_ring_phys;

    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

  if (!configure_interrupts())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to configure suitable interrupt system\n");
    set_device_status(OPER_STATUS::FAILED);
  }

  // Remaining steps are left for child classes.
  // 3 - Negotiate features.
  // 4 - Device specific setup.
  // 5 - Set device OK.

  KL_TRC_EXIT;
}

generic_device::~generic_device()
{
  KL_TRC_ENTRY;

  for (uint64_t addr : mapped_phys_addrs)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Deallocate range: ", addr, "\n");
    mem_deallocate_virtual_range(reinterpret_cast<void *>(addr), 1);
  }

  if (interrupt_number != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unregister interrupts\n");
    proc_unregister_irq_handler(interrupt_number, this);
  }

  KL_TRC_EXIT;
}

/// @brief Read the PCI configuration space for this device.
///
/// @return True if the configuration was loaded successfully. False otherwise.
bool generic_device::read_pci_config()
{
  bool result{true};
  pci_cap virtio_cap;
  bool pci_cfg_found{false};
  uint32_t extra_offset{0};

  KL_TRC_ENTRY;

  for (auto c : this->caps.vendor_specific)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Vendor specific cap @ ", c.offset, "\n");
    if (!read_capability_block(c, &virtio_cap, sizeof(virtio_cap)))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read capability block\n");
      result = false;
      break;
    }

    switch(static_cast<CONFIG_STRUCTURE_TYPES>(virtio_cap.cfg_type))
    {
    case CONFIG_STRUCTURE_TYPES::COMMON_CFG:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Common config block\n");
      if (!map_config_block(virtio_cap, reinterpret_cast<void **>(&common_cfg)))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to store common config buffer\n");
        result = false;
      }
      break;

    case CONFIG_STRUCTURE_TYPES::NOTIFY_CFG:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Notification config block\n");
      extra_offset = c.offset + sizeof(virtio_cap);
      notify_offset_mult = pci_read_raw_reg(this->_address, extra_offset / 4);

      if (!map_config_block(virtio_cap, &notification_base_addr))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to store notification config\n");
        result = false;
      }

      KL_TRC_TRACE(TRC_LVL::FLOW, "Notifaction base address reg: ", virtio_cap.bar, "\n");
      KL_TRC_TRACE(TRC_LVL::FLOW, "Notification offset: ", virtio_cap.offset, "\n");
      KL_TRC_TRACE(TRC_LVL::FLOW, "Notification offset multiplier: ", notify_offset_mult, "\n");
      break;

    case CONFIG_STRUCTURE_TYPES::ISR_CFG:
      KL_TRC_TRACE(TRC_LVL::FLOW, "ISR config block\n");
      if (!map_config_block(virtio_cap, reinterpret_cast<void **>(&isr_status_byte)))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to store notification config\n");
        result = false;
      }

      break;

    case CONFIG_STRUCTURE_TYPES::DEVICE_CFG:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Device-specific config block\n");
      if (!map_config_block(virtio_cap, reinterpret_cast<void **>(&device_cfg_void)))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to store device config buffer\n");
        result = false;
      }
      break;

    case CONFIG_STRUCTURE_TYPES::PCI_CFG:
      KL_TRC_TRACE(TRC_LVL::FLOW, "PCI config block\n");
      pci_cfg_found = true;
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown config type (", virtio_cap.cfg_type, ") - skip\n");
      break;
    }
  }

  if (!pci_cfg_found)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Didn't find PCI config => legacy device. Not supported\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Given a virtio Capability block, locate the relevant configuration buffer and return a pointer to it.
///
/// @param cap The capability block referring to a configuration buffer.
///
/// @param[out] config_block Where to store the pointer to the configuration buffer.
///
/// @return true on success, false on failure.
bool generic_device::map_config_block(pci_cap cap, void **config_block)
{
  bool result{true};
  uint64_t physical_addr;
  void *virtual_addr;
  uint64_t page{0};
  uint64_t offset{0};

  KL_TRC_ENTRY;

  if (config_block == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "config_block ptr invalid\n");
    result = false;
  }
  else
  {
    physical_addr = (pci_read_base_addr_reg(this->_address, cap.bar) & 0xFFFFFFFFFFFFFFF0) + cap.offset;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Physical address: ", physical_addr, "\n");

    virtual_addr = mem_allocate_virtual_range(1);
    klib_mem_split_addr(physical_addr, page, offset);
    mem_map_range(reinterpret_cast<void *>(page), virtual_addr, 1, nullptr, MEM_CACHE_MODES::MEM_UNCACHEABLE);

    mapped_phys_addrs.push_back(reinterpret_cast<uint64_t>(virtual_addr));

    *config_block = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(virtual_addr) + offset);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Config block stored at: ", *config_block, "\n");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Read the device's device feature bits.
///
/// @return the current device feature bits.
uint64_t generic_device::read_feature_bits()
{
  uint64_t result{0};

  KL_TRC_ENTRY;

  common_cfg->device_feature_select = 1;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  result = common_cfg->device_feature;
  result <<= 32;

  std::atomic_thread_fence(std::memory_order_seq_cst);
  common_cfg->device_feature_select = 0;

  std::atomic_thread_fence(std::memory_order_seq_cst);
  result |= common_cfg->device_feature;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Write the device's driver feature bits.
///
/// Note that read_feature_bits() reads the device feature bits, but this writes the driver feature bits.
///
/// @param new_features The feature bits to write.
void generic_device::write_feature_bits(uint64_t new_features)
{
  KL_TRC_ENTRY;

  common_cfg->driver_feature_select = 0;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  common_cfg->driver_feature = static_cast<uint32_t>(new_features);
  new_features >>= 32;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  common_cfg->driver_feature_select = 1;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  common_cfg->driver_feature = static_cast<uint32_t>(new_features);

  KL_TRC_EXIT;
}

/// @brief Attempt to negotiate a set of feature bits with the device.
///
/// This performs a very simple negotiation, child drivers may prefer something more complex. It is possible to call
/// this function with incompatible sets of feature bits - for example, the same bit being set in required_on and
/// required_off. It does not attempt to resolve this, the results are not well defined but are not harmful.
///
/// @param required_on Feature bits that must be set for negotiation to be successful.
///
/// @param required_off Feature bits that must be unset for negotiation to be successful. This value is unused in this
///                     simple negotiation, but might be useful in more complex scenarios.
///
/// @param optional_on Feature bits that the driver would prefer to be on, but which would not cause negotiation to
///                    fail.
///
/// @param optional_off Feature bits that the driver would prefer to be off, but which would not cause negotiation to
///                     fail. This value is unused in this simple negotiation, but might be useful in more complex
///                     scenarios.
///
/// @return True if negotiation was successful, false otherwise.
bool generic_device::negotiate_features(uint64_t required_on,
                                        uint64_t required_off,
                                        uint64_t optional_on,
                                        uint64_t optional_off)
{
  bool result{true};
  uint64_t current_fbs;
  uint64_t preferred_bits;
  uint32_t temp_status;

  KL_TRC_ENTRY;

  current_fbs = read_feature_bits();

  // Check that all required bits are supported:
  if((~current_fbs & required_on) != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Some required bits not supported\n");
    result = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "All required bits supported\n");

    // Now get the subset of optional bits that's supported
    optional_on = optional_on & current_fbs;

    // Write our desired features to the device.
    preferred_bits = required_on | optional_on;
    write_feature_bits(preferred_bits);

    // Check that this is acceptable.
    temp_status = common_cfg->device_status;
    temp_status |= OPER_STATUS_BITS::FEATURES_OK;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    common_cfg->device_status = temp_status;
    std::atomic_thread_fence(std::memory_order_seq_cst);

    result = ((common_cfg->device_status & OPER_STATUS_BITS::FEATURES_OK) != 0);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Configure the interrupt system for each virtqueue.
///
/// When the system supports MSI-X, use that here. For now we have to use 'classic' interrupts.
///
/// @return True if interrupts successfully configured. False otherwise.
bool generic_device::configure_interrupts()
{
  bool result{false};
  pci_reg_15 interrupt_reg;

  KL_TRC_ENTRY;

  interrupt_reg.raw = pci_read_raw_reg(_address, PCI_REGS::LATS_AND_INTERRUPTS);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Pin: ", interrupt_reg.interrupt_pin, "\n");
  interrupt_number = compute_irq_for_pin(interrupt_reg.interrupt_pin - 1);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Computed interrupt: ", interrupt_number, "\n");
  proc_register_irq_handler(interrupt_number, this);

  result = true;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result,"\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Set the driver OK bit.
///
/// This indicates that the driver has completed setup.
void generic_device::set_driver_ok()
{
  uint32_t device_status;
  KL_TRC_ENTRY;

  device_status = common_cfg->device_status;
  device_status |= OPER_STATUS_BITS::DRIVER_OK;
  common_cfg->device_status = device_status;
  std::atomic_thread_fence(std::memory_order_seq_cst);

  KL_TRC_EXIT;
}

/// @brief Enable all queues associated with this device.
void generic_device::enable_queues()
{
  KL_TRC_ENTRY;

  for (uint16_t i = 0; i < queues.size(); i++)
  {
    common_cfg->queue_select = i;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    common_cfg->queue_enable = 1;
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

  KL_TRC_EXIT;
}

/// @brief Disable all queues associated with this device.
void generic_device::disable_queues()
{
  KL_TRC_ENTRY;

  for (uint16_t i = 0; i < queues.size(); i++)
  {
    common_cfg->queue_select = i;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    common_cfg->queue_enable = 0;
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

  KL_TRC_EXIT;
}

/// @brief Cause the available buffers queue to be emptyied.
///
/// Not currently used.
void generic_device::empty_avail_queue()
{
  INCOMPLETE_CODE("empty_avail_queue");
}

/// @brief Notify the device that the available queue has buffers available for it.
///
/// The calculation here comes directly from the virtio spec
///
/// @param queue_number The index of the queue to notify.
///
/// @param next_index The next (currently unused) index in the queue.
void generic_device::notify_avail_buffers(uint16_t queue_number, uint16_t next_index)
{
  KL_TRC_ENTRY;

  common_cfg->queue_select = queue_number;

  std::atomic_thread_fence(std::memory_order_seq_cst);
  uint64_t addr = reinterpret_cast<uint64_t>(notification_base_addr) +
                  (common_cfg->queue_notify_off * notify_offset_mult);
  uint16_t *ptr = reinterpret_cast<uint16_t *>(addr);
  *ptr = queue_number;
  std::atomic_thread_fence(std::memory_order_seq_cst);

  KL_TRC_EXIT;
}

bool generic_device::handle_translated_interrupt_fast(uint8_t interrupt_offset,
                                                      uint8_t raw_interrupt_num)
{
  bool slow_interrupt_req{false};
  uint32_t isr_field;

  KL_TRC_ENTRY;

  // We only support the old ways at the moment, so see whether an interrupt occurred.
  isr_field = *isr_status_byte;

  if (isr_field & 1)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Queue interrupt\n");
    slow_interrupt_req = true;
  }

  if (isr_field & 2)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Configuration interrupt\n");
    slow_interrupt_req = true;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Slow interrupt required? ", slow_interrupt_req, "\n");
  KL_TRC_EXIT;

  return slow_interrupt_req;
}

void generic_device::handle_translated_interrupt_slow(uint8_t interrupt_offset,
                                                      uint8_t raw_interrupt_num)
{
  KL_TRC_ENTRY;

  // Examine all used queues to look for new buffers.
  for (virtqueue &q : queues)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Processing queue ", &q, "\n");
    q.process_used_ring();
  }

  // For the time being we completely ignore the possibility that the device configuration might have changed...

  KL_TRC_EXIT;
}

};
