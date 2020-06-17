/// @file
/// @brief Implements a generic USB device core.

//#define ENABLE_TRACING

#include "kernel_all.h"
#include "usb_gen_device.h"
#include "usb.h"

using namespace usb;

/// @brief Retrieve a device descriptor.
///
/// This function will block until completion.
///
/// @param descriptor_type The type of descriptor to retrieve - standard descriptor constants are given in the
///                        DESCRIPTOR_TYPES namespace.
///
/// @param idx The index of the descriptor to retrieve. Zero is always valid, other numbers may not be.
///
/// @param language_id The language of the descriptor to retrieve. Zero is always valid, other values depend on the
///                    descriptor type and whether the device supports multiple languages.
///
/// @param length How many bytes of descriptor to read.
///
/// @param data_ptr_virt Pointer to a memory buffer to store the results in. (A virtual space address, rather than a
///                      physical one) May only be nullptr if length is zero.
///
/// @param request_type_raw The Request Type value to send
///
/// @return True if the descriptor could be read as requested, false otherwise.
bool generic_core::get_descriptor(uint8_t descriptor_type,
                                  uint8_t idx,
                                  uint16_t language_id,
                                  uint16_t length,
                                  void *data_ptr_virt,
                                  uint8_t request_type_raw)
{
  bool result;
  device_request_type rt;
  uint16_t value;

  KL_TRC_ENTRY;

  rt.raw = request_type_raw;
  value = descriptor_type;
  value <<= 8;
  value |= idx;

  result = this->device_request(rt, DEV_REQUEST::GET_DESCRIPTOR, value, language_id, length, data_ptr_virt);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Send a SET_CONFIGURATION request to the device
///
/// @param config_num The index into the configurations array for the config to set. Note: Not the value that is sent
///                   over the wire, which may be different.
///
/// @return True if successful, false otherwise.
bool generic_core::set_configuration(uint8_t config_num)
{
  bool result = false;
  device_request_type rt;

  KL_TRC_ENTRY;

  rt.raw = 0;

  if (config_num < this->main_device_descriptor.num_configurations)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Valid config number\n");
    result = this->device_request(rt,
                                  DEV_REQUEST::SET_CONFIGURATION,
                                  configurations[config_num].desc.config_index_number,
                                  0,
                                  0,
                                  nullptr);

    if (result)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Update config number\n");
      active_configuration = config_num;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid config number\n");
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Do all the generic device configuration.
///
/// For example, retrieve device and interface descriptors and update the maximum packet size, if needed.
///
/// After this call, the device should be in the configured state.
///
/// @return true if the discovery process completed successfully. False otherwise.
bool generic_core::do_device_discovery()
{
  bool result = true;
  KL_TRC_ENTRY;

  current_state = CORE_STATE::READING_DEV_DESCRIPTOR;
  result = read_device_descriptor();

  if (!result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read main device descriptor\n");
    current_state = CORE_STATE::FAILED;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Read the USB device descriptor in to main_device_descriptor.
///
/// If necessary, update the maximum packet size to achieve this.
///
/// @return True if this operation succeeded, false otherwise.
bool generic_core::read_device_descriptor()
{
  bool result;

  KL_TRC_ENTRY;

  result = get_descriptor(DESCRIPTOR_TYPES::DEVICE, 0, 0, sizeof(device_descriptor), &main_device_descriptor);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Read a USB device's config descriptor for a given config number.
///
/// @param index The configuration number to read (valid values are zero to one less than the device descriptor field
///              num_configurations)
///
/// @param[out] config The config storage to write the descriptor in to.
///
/// @return True if the descriptor was read successfully, false otherwise.
bool generic_core::read_config_descriptor(uint8_t index, device_config &config)
{
  bool result = false;

  KL_TRC_ENTRY;

  config.raw_descriptor = std::make_unique<uint8_t[]>(sizeof(config_descriptor));
  config.raw_descriptor_length = sizeof(config_descriptor);
  result = get_descriptor(DESCRIPTOR_TYPES::CONFIGURATION,
                          index,
                          0,
                          sizeof(config_descriptor),
                          config.raw_descriptor.get());

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void generic_core::handle_message(std::unique_ptr<msg::root_msg> &message)
{
  KL_TRC_ENTRY;

  switch (message->message_id)
  {
  case SM_USB_TRANSFER_COMPLETE:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer complete message\n");
    {
      transfer_complete_msg *msg = dynamic_cast<transfer_complete_msg *>(message.get());
      ASSERT(msg);
      handle_transfer_complete(msg);
    }
    break;

  default:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown message (", message->message_id, ") ignore\n");
    break;
  }

  KL_TRC_EXIT;
}

/// @brief Called when a transfer initiated by this device core is complete.
///
/// @param message The transfer complete message for the transfer that completed.
void generic_core::handle_transfer_complete(transfer_complete_msg *message)
{
  KL_TRC_ENTRY;

  ASSERT(message);

  switch(current_state)
  {
  case CORE_STATE::READING_DEV_DESCRIPTOR:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read dev descriptor complete\n");
    got_device_descriptor();
    break;

  case CORE_STATE::READING_CFG_DESCRIPTORS:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read a config descriptor\n");
    got_config_descriptor();
    break;

  case CORE_STATE::SETTING_CONFIG:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Config set\n");
    configuration_set();
    break;

  default:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown state - ignore\n");
    break;
  }

  KL_TRC_EXIT;
}

/// @brief Called when a transfer carrying this device's Device Descriptor has completed.
///
/// This means we can move on to trying to get the config descriptors.
void generic_core::got_device_descriptor()
{
  uint32_t new_max_packet_size;
  bool result;

  KL_TRC_ENTRY;

  new_max_packet_size = 1 << main_device_descriptor.max_packet_size_encoded;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Actual max packet size: ", new_max_packet_size, "\n");

  // If the max packet size given by the device doesn't match the packet size we were using, update it. It's also
  // possible that we failed to read the entire device descriptor in that case, so re-read it.
  if (new_max_packet_size != get_max_packet_size())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Update maximum packet size\n");
    result = set_max_packet_size(new_max_packet_size);
  }
  else
  {
    current_state = CORE_STATE::READING_CFG_DESCRIPTORS;
    configs_read = 0;
    configurations = std::unique_ptr<device_config[]>(new device_config[main_device_descriptor.num_configurations]);
    result = read_config_descriptor(0, configurations[0]);
  }

  if (!result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Command failed\n");
    current_state = CORE_STATE::FAILED;
  }

  KL_TRC_EXIT;
}

/// @brief Called when the maximum packet size for this device has been updated
///
/// This occurs as part of reading the device descriptor. Indeed, it may mean that the device descriptor was previously
/// truncated, so attempt to read it again.
void generic_core::set_max_packet_size_complete()
{
  bool result;

  KL_TRC_ENTRY;
  current_state = CORE_STATE::READING_DEV_DESCRIPTOR;
  result = read_device_descriptor();

  if (!result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read device descriptor failed\n");
    current_state = CORE_STATE::FAILED;
  }

  KL_TRC_EXIT;
}

/// @brief Called when a config descriptor, or part of one, has been received.
///
/// Unfortunately, receiving a config descriptor is always a two-step process, since we don't know how many interfaces
/// and endpoints will be specified by the descriptor. So, grab the descriptor once, then update the size of the buffer
/// and grab it again.
void generic_core::got_config_descriptor()
{
  config_descriptor *active_descriptor;
  device_config *active_cfg;
  bool result{false};

  KL_TRC_ENTRY;

  active_cfg = &configurations[configs_read];
  ASSERT(active_cfg);

  active_descriptor = reinterpret_cast<config_descriptor *>(active_cfg->raw_descriptor.get());
  ASSERT(active_descriptor);

  if (active_cfg->raw_descriptor_length == active_descriptor->total_length)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully read descriptor #", configs_read,
                                " (", active_descriptor->total_length, " bytes)\n");

    result = interpret_raw_descriptor(configs_read);

    if (result)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Interpreted descriptor ", configs_read, "\n");
      ++configs_read;

      if (configs_read == main_device_descriptor.num_configurations)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "All config descriptors read...\n");
        usb::main_factory::create_device(self_weak_ptr.lock(), usb::main_factory::CREATION_PHASE::DISCOVERY_COMPLETE);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Start reading descriptor ", configs_read, "\n");
        result = read_config_descriptor(configs_read, configurations[configs_read]);
      }
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Update size of descriptor ", configs_read,
                                " to ", active_descriptor->total_length, " and retry\n");

    active_cfg->raw_descriptor = std::make_unique<uint8_t[]>(active_descriptor->total_length);
    active_cfg->raw_descriptor_length = active_descriptor->total_length;
    result = get_descriptor(DESCRIPTOR_TYPES::CONFIGURATION,
                            configs_read,
                            0,
                            active_descriptor->total_length,
                            active_cfg->raw_descriptor.get());
  }

  if (!result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read config descriptor\n");
    current_state = CORE_STATE::FAILED;
  }

  KL_TRC_EXIT;
}

/// @brief Interpret a newly read config desciptor
///
/// @param config_num The configuration index to interpret
///
/// @return True if the descriptor could be interpreted successfully, false otherwise.
bool generic_core::interpret_raw_descriptor(uint64_t config_num)
{
  device_config &config{configurations[config_num]};
  config_descriptor *desc;
  bool result{true};

  KL_TRC_ENTRY;

  ASSERT(&config);

  klib_list_initialize(&config.other_descriptors);

  uint8_t *raw_ptr;
  uint16_t raw_ptr_offset;
  uint8_t current_interface;
  uint8_t current_endpoint;
  bool found_an_interface_before = false;
  descriptor_header *hdr;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Handle config desc. #", config_num, " (", config.raw_descriptor_length, " bytes)\n");
  ASSERT(config.raw_descriptor);
  desc = reinterpret_cast<config_descriptor *>(config.raw_descriptor.get());
  raw_ptr = reinterpret_cast<uint8_t *>(config.raw_descriptor.get());

  memcpy(&config.desc, desc, sizeof(config_descriptor));
  raw_ptr += sizeof(config_descriptor);
  raw_ptr_offset = sizeof(config_descriptor);

  config.interfaces = std::make_unique<device_interface[]>(desc->num_interfaces);
  current_interface = 0;
  current_endpoint = 0;

  while ((raw_ptr_offset < config.raw_descriptor_length) && (result))
  {
    hdr = reinterpret_cast<descriptor_header *>(raw_ptr);
    KL_TRC_TRACE(TRC_LVL::FLOW, "New header. Type: ", hdr->descriptor_type, ", length: ", hdr->length, "\n");

    switch (hdr->descriptor_type)
    {
    case DESCRIPTOR_TYPES::INTERFACE:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found interface descriptor\n");

      if (found_an_interface_before && (current_endpoint != config.interfaces[current_interface].desc.num_endpoints))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found an interface before finding all expected endpoints!\n");
        result = false;
        break;
      }

      if (found_an_interface_before)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Move on to next interface\n");
        current_interface++;
      }

      found_an_interface_before = true;
      memcpy(&(config.interfaces.get()[current_interface].desc), raw_ptr, sizeof(interface_descriptor));
      current_endpoint = 0;
      klib_list_initialize(&config.interfaces[current_interface].other_descriptors);

      KL_TRC_TRACE(TRC_LVL::FLOW, "Make storage for ", config.interfaces[current_interface].desc.num_endpoints, " endpoints\n");
      config.interfaces[current_interface].endpoints =
        std::make_unique<endpoint_descriptor[]>(config.interfaces[current_interface].desc.num_endpoints);

      break;

    case DESCRIPTOR_TYPES::ENDPOINT:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found endpoint descriptor\n");

      if ((!found_an_interface_before) ||
          (current_endpoint == config.interfaces[current_interface].desc.num_endpoints))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Either found an endpoint before an interface, or found too many endpoints\n");
        result = false;
        break;
      }

      memcpy(&(config.interfaces[current_interface].endpoints.get()[current_endpoint]),
            raw_ptr,
            sizeof(endpoint_descriptor));
      current_endpoint++;

      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found a descriptor of type: ", hdr->descriptor_type);

      klib_list_item<descriptor_header *> *new_item = new klib_list_item<descriptor_header *>;
      klib_list_item_initialize(new_item);
      new_item->item = hdr;

      if (!found_an_interface_before)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, " - attach to config\n");
        klib_list_add_tail(&config.other_descriptors, new_item);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, " - attach to interface ", current_interface, "\n");
        klib_list_add_tail(&config.interfaces[current_interface].other_descriptors, new_item);
      }
    };

    raw_ptr += hdr->length;
    raw_ptr_offset += hdr->length;
  }

  if (current_endpoint != config.interfaces[current_interface].desc.num_endpoints)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Missing endpoints for final interface\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
