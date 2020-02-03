/// @file
/// @brief Implements a generic USB device core.

//#define ENABLE_TRACING

#include "devices/usb/usb_gen_device.h"
#include "klib/klib.h"

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
/// @return true if the discovery process completed succesfully. False otherwise.
bool generic_core::do_device_discovery()
{
  bool result = true;
  KL_TRC_ENTRY;

  result = read_device_descriptor();

  if (!result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read main device descriptor\n");
  }
  else
  {
    if (main_device_descriptor.num_configurations == 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid number of configurations.\n");
      result = false;
    }
    else
    {
      configurations = std::unique_ptr<device_config[]>(new device_config[main_device_descriptor.num_configurations]);

      for (uint8_t i = 0; i < main_device_descriptor.num_configurations; i++)
      {
        if (!read_config_descriptor(i, configurations[i]))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read config ", i, "\n");
          result = false;
          break;
        }
      }
    }
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
  bool result = false;
  uint32_t new_max_packet_size;

  KL_TRC_ENTRY;

  result = get_descriptor(DESCRIPTOR_TYPES::DEVICE, 0, 0, sizeof(device_descriptor), &main_device_descriptor);
  if (!result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to get device descriptor (1)\n");
  }
  else
  {
    new_max_packet_size = 1 << main_device_descriptor.max_packet_size_encoded;
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Actual max packet size: ", new_max_packet_size, "\n");

    // If the max packet size given by the device doesn't match the packet size we were using, update it. It's also
    // possible that we failed to read the entire device descriptor in that case, so re-read it.
    if (new_max_packet_size != get_max_packet_size())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Update maximum packet size\n");
      result = set_max_packet_size(new_max_packet_size);
      if (!result)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to set maximum packet size\n");
      }
      else
      {
        result = get_descriptor(DESCRIPTOR_TYPES::DEVICE, 0, 0, sizeof(device_descriptor), &main_device_descriptor);
      }
    }
  }

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

  std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(sizeof(config_descriptor));
  config_descriptor *desc;
  uint16_t true_length;
  uint8_t *raw_ptr;
  uint16_t raw_ptr_offset;
  uint8_t current_interface;
  uint8_t current_endpoint;
  bool found_an_interface_before = false;
  descriptor_header *hdr;

  KL_TRC_ENTRY;

  // Unfortunately, this is always a two-step process, since we don't know how many interfaces and endpoints will be
  // specified by the descriptor. So, grab the descriptor once, then update the size of the buffer and grab it again.
  result = get_descriptor(DESCRIPTOR_TYPES::CONFIGURATION, index, 0, sizeof(config_descriptor), buffer.get());
  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Still OK so far.");
    desc = reinterpret_cast<config_descriptor *>(buffer.get());
    true_length = desc->total_length;
    desc = nullptr;

    config.raw_descriptor = std::make_unique<uint8_t[]>(true_length);
    result = get_descriptor(DESCRIPTOR_TYPES::CONFIGURATION, index, 0, true_length, config.raw_descriptor.get());
  }

  klib_list_initialize(&config.other_descriptors);

  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully grabbed the descriptor\n");
    ASSERT(config.raw_descriptor);
    desc = reinterpret_cast<config_descriptor *>(config.raw_descriptor.get());
    raw_ptr = reinterpret_cast<uint8_t *>(config.raw_descriptor.get());

    memcpy(&config.desc, desc, sizeof(config_descriptor));
    raw_ptr += sizeof(config_descriptor);
    raw_ptr_offset = sizeof(config_descriptor);

    config.interfaces = std::make_unique<device_interface[]>(desc->num_interfaces);
    current_interface = 0;
    current_endpoint = 0;

    while ((raw_ptr_offset < true_length) && (result))
    {
      hdr = reinterpret_cast<descriptor_header *>(raw_ptr);

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

    desc = nullptr;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
