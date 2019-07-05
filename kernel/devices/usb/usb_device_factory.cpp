/// @file
/// @brief Implements a USB device factory.

// Known defects:
// - USB device objects simply go into a tree instead of anywhere sensible. This also means device disconnection
//   doesn't really work very well...

//#define ENABLE_TRACING

// Core includes
#include "klib/klib.h"
#include "devices/usb/usb.h"

// Known USB devices:
#include "devices/usb/hid/usb_hid_device.h"

namespace
{
  std::shared_ptr<usb::main_factory> *factory = nullptr; ///< The Singleton USB device factory for the system.

  // This is a way to keep hold of USB devices until the device tree is more well developed. In the future USB devices
  // will be children of a reasonable parent device or controller, which is a child of a PCI device, and so on.
  kl_rb_tree<uint64_t, std::shared_ptr<usb::generic_device>> *devices = nullptr; ///< TEMPO tree of known devices.
  volatile uint64_t num_devices = 0; ///< TEMPO number of devices previously created.
  kernel_spinlock tree_lock = 0; ///< Lock protecting the devices tree.

  kernel_spinlock factory_create_spinlock = 0;

#ifdef ENABLE_TRACING
  void trace_device_descriptors(std::shared_ptr<usb::generic_core> core);
#endif

  uint8_t select_configuration(std::shared_ptr<usb::generic_core> core);
}

namespace usb {

/// @brief Initialise the USB system.
///
/// In practice, this means creating a device factory and a way of remembering all inserted USB devices.
void initialise_usb_system()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(factory_create_spinlock);
  if (factory == nullptr)
  {
    factory = new std::shared_ptr<main_factory>;
    *factory = std::make_shared<main_factory>();

    devices = new kl_rb_tree<uint64_t, std::shared_ptr<usb::generic_device>>();
  }
  klib_synch_spinlock_unlock(factory_create_spinlock);

  KL_TRC_EXIT;
}

/// @brief Queue a device creation request.
///
/// This takes an initialised device core and creates the generic part of the driver to drive it.
///
/// @param device_core The USB controller-specific device core to create a driver around.
void main_factory::create_device(std::shared_ptr<generic_core> device_core)
{
  KL_TRC_ENTRY;

  std::shared_ptr<create_device_work_item> item = std::make_shared<create_device_work_item>(device_core);
  work::queue_work_item(*factory, item);

  KL_TRC_EXIT;
}

/// @brief Called to create a device asynchronously.
///
/// @param item The queued work item - this must be a USB device creation request.
void main_factory::handle_work_item(std::shared_ptr<work::work_item> item)
{
  KL_TRC_ENTRY;

  // Which route we take depends on what work item has been given to us!
  std::shared_ptr<create_device_work_item> cd_item = std::dynamic_pointer_cast<create_device_work_item>(item);
  if (cd_item)
  {
    create_device_handler(cd_item);
  }
  else
  {
    panic("Unknown USB factory work item.");
  }

  KL_TRC_EXIT;
}

/// @brief Called when a device is being asynchronously initialised.
///
/// @param item The request containing the device core to create a driver around.
void main_factory::create_device_handler(std::shared_ptr<create_device_work_item> item)
{
  std::shared_ptr<generic_core> device_core = item->device_core;
  std::shared_ptr<generic_device> new_device;
  uint8_t config_idx;

  KL_TRC_ENTRY;

  device_core->do_device_discovery();

#ifdef ENABLE_TRACING
  trace_device_descriptors(device_core);
#endif

  config_idx = select_configuration(device_core);
  if (!device_core->configure_device(config_idx))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device creation failed\n");
  }
  else
  {
    // At some time in the future, we will support looking up whole devices, but not just yet - we just do interfaces.
    for (uint8_t i = 0; i < device_core->configurations[config_idx].desc.num_interfaces; i++)
    {
      // There are no specific USB devices yet...
      switch(device_core->configurations[config_idx].interfaces[i].desc.interface_class)
      {
      case DEVICE_CLASSES::HID:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Human interface device\n");
        new_device = std::make_shared<hid_device>(device_core, i);
        break;

      default:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown device type.\n");
        new_device = std::make_shared<generic_device>(device_core, i, "Unrecognised USB Device");
        break;
      }
    }

    klib_synch_spinlock_lock(tree_lock);
    devices->insert(num_devices, new_device);
    num_devices++;
    klib_synch_spinlock_unlock(tree_lock);
  }

  KL_TRC_EXIT;
}

/// @brief Simple constructor
///
/// @param core The core to store in this work item.
main_factory::create_device_work_item::create_device_work_item(std::shared_ptr<generic_core> core) :
  device_core{core}
{
  // Deliberately no actions.
}

}; // USB namespace.

namespace
{
#ifdef ENABLE_TRACING
  /// @brief Trace out the device descriptor for a given USB device core.
  ///
  /// Useful for debugging purposes.
  ///
  /// @param core The core to read the descriptors from.
  void trace_device_descriptors(std::shared_ptr<usb::generic_core> core)
  {
    KL_TRC_ENTRY;
    KL_TRC_TRACE(TRC_LVL::FLOW, "New device details: \n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "USB Version: ", core->main_device_descriptor.usb_ver_bcd, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "Class / subclass / proto: ",
                                core->main_device_descriptor.device_class, " / ",
                                core->main_device_descriptor.device_subclass, " / ",
                                core->main_device_descriptor.device_protocol, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "Vendor / Product ID: ",
                                core->main_device_descriptor.vendor_id, " / ",
                                core->main_device_descriptor.product_id, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "Max packet size (coded): ",core->main_device_descriptor.max_packet_size_encoded,"\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "Number of configurations: ", core->main_device_descriptor.num_configurations, "\n");

    KL_TRC_TRACE(TRC_LVL::FLOW, "-------------------------\n");

    for (uint8_t i = 0; i < core->main_device_descriptor.num_configurations; i++)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Config ", i, " (idx: ", core->configurations[i].desc.config_index_number, ")\n");
      KL_TRC_TRACE(TRC_LVL::FLOW, "Number of interfaces: ", core->configurations[i].desc.num_interfaces, "\n");

      for (uint8_t j = 0; j < core->configurations[i].desc.num_interfaces; j++)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "- Interface: ", j,
                                    " (idx: ", core->configurations[i].interfaces[j].desc.interface_number, ")\n");
        KL_TRC_TRACE(TRC_LVL::FLOW, " - Class/subclass/protocol: ",
                                    core->configurations[i].interfaces[j].desc.interface_class, " / ",
                                    core->configurations[i].interfaces[j].desc.interface_subclass, " / ",
                                    core->configurations[i].interfaces[j].desc.interface_protocol, "\n");
        KL_TRC_TRACE(TRC_LVL::FLOW, " - Number of endpoints: ",
                                    core->configurations[i].interfaces[j].desc.num_endpoints, "\n");
        for (int k = 0; k < core->configurations[i].interfaces[j].desc.num_endpoints; k++)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, " - ", k, ": ",
                       core->configurations[i].interfaces[j].endpoints[k].attributes.raw, " - ",
                       core->configurations[i].interfaces[j].endpoints[k].endpoint_address, " - ",
                       core->configurations[i].interfaces[j].endpoints[k].max_packet_size, " - ",
                       core->configurations[i].interfaces[j].endpoints[k].service_interval, "\n");
        }
      }
    }

    KL_TRC_EXIT;
  }
#endif

  /// @brief Choose which of the configurations a device supports to configure it as.
  ///
  /// Some USB devices support more than one configuration with differing capabilities. This function will, eventually,
  /// choose the system's preferred configuration. But, for the time being it simply returns the index of the first
  /// configuration.
  ///
  /// @param core The device core to examine.
  ///
  /// @return The configuration index to be sent to set_configuration. Note, not the index that the device itself
  ///         refers to its configurations as.
  uint8_t select_configuration(std::shared_ptr<usb::generic_core> core)
  {
    uint8_t config_idx = 0;

    KL_TRC_ENTRY;

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", config_idx, "\n");
    KL_TRC_EXIT;

    return config_idx;
  }

} // Local namespace
