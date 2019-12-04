/// @file
/// @brief Definitions of a generic USB device.
///
/// There are two parts - the "main" part that implements a consistent interface with the outside world, and a "core"
/// part that interfaces with the controller that the device is connected to.
//
// Known defects:
// - We pass around work_response objects that are specifically intended to store the results of work, but just use
//   them for waiting. This seems odd.

#pragma once

#include <memory>

#include "devices/device_interface.h"
#include "devices/usb/usb_gen_device_requests.h"
#include "processor/work_queue.h"
#include "processor/synch_objects.h"

#include "klib/data_structures/lists.h"

namespace usb
{
  /// @brief Structure representing an interface within a device configuration.
  ///
  struct device_interface
  {
    interface_descriptor desc; ///< Descriptor for this interface.
    std::unique_ptr<endpoint_descriptor[]> endpoints; ///< Each of the endpoints described by this interface.
    klib_list<descriptor_header *> other_descriptors; ///< Other descriptors that appear to be part of this interface.
  };

  /// @brief Structure representing a single device configuration.
  ///
  struct device_config
  {
    config_descriptor desc; ///< Descriptor for this configuration.
    std::unique_ptr<device_interface[]> interfaces; ///< Interfaces used by this configuration.

    /// Other descriptors that appear to be part of this configuration. If the configuration descriptor contains
    /// additional descriptors that appear after an interface descriptor, they will be considered part of that
    /// interface instead.
    ///
    /// The descriptor pointers in this list will point in to the buffer of raw_descriptor.
    klib_list<descriptor_header *> other_descriptors;

    std::unique_ptr<uint8_t[]> raw_descriptor; ///< Copy of the descriptor in raw format.
  };

  /// @brief Known device classes that could populate the "Class" field.
  enum DEVICE_CLASSES
  {
    DEVICE = 0, ///< Class/subclass are given in the interface fields.
    AUDIO = 1, ///< Audio device
    COMMUNICATIONS = 2, ///< Communications device or device controller.
    HID = 3, ///< Human Interface Device.
    PHYSICAL = 5, ///< Physical interface device.
    IMAGE = 6, ///< Imaging device.
    PRINTER = 7, ///< Printer
    MASS_STORAGE = 8, ///< Mass storage device.
    HUB = 9, ///< USB Hub.
    CDC_DATA = 10, ///< Communications device data part
    SMART_CARD = 11, ///< Smart Card device
    CONTENT_SECURITY = 13, ///< DRM device.
    VIDEO = 14, ///< Video device.
    PERSONAL_HEALTHCARE = 15, ///< 'Personal Healthcare' device...
    AV = 16, ///< Combined audio-visual device.
    BILLBOARD = 17, ///< Billboard-type device.
    TYPE_C_BRIDGE = 18, ///< Type-C bridge
    DIAGNOSTIC_DEVICE = 0xDC, ///< Diagnostics-output device
    WIRELESS_CONTROLLER = 0xE0, ///< Wireless (bluetooth, etc.) controller.
    MISCELLANEOUS = 0xEF, ///< Other devices.
  };

  class generic_device;

  /// @brief Stores details of a normal (non-command) USB transfer.
  ///
  class normal_transfer : public WaitForFirstTriggerObject
  {
  public:
    normal_transfer(generic_device *owner, std::unique_ptr<uint8_t[]> buffer, uint32_t length);
    virtual ~normal_transfer();
    virtual void owner_dying();

    /// @brief Trigger waiting threads to continue, but also signal the owner object that this transfer is complete.
    ///
    virtual void set_response_complete() { this->trigger_all_threads(); };

    std::unique_ptr<uint8_t[]> transfer_buffer; ///< The buffer containing the transfer to either send or receive.
    uint32_t buffer_size; ///< The number of bytes in the buffer.

  protected:
    generic_device *owner_device; ///< The USB device that this transfer belongs to.
  };

  /// @brief Generic USB device core.
  ///
  class generic_core
  {
  public:
    virtual ~generic_core() = default;

    // Non-pure-virtual members are described in usb_gen_device_core.cpp.
    virtual bool do_device_discovery();

    virtual bool read_device_descriptor();
    virtual bool read_config_descriptor(uint8_t index, device_config &config);

    // Standard device requests.
    virtual bool get_descriptor(uint8_t descriptor_type,
                                uint8_t idx,
                                uint16_t language_id,
                                uint16_t length,
                                void *data_ptr_virt,
                                uint8_t request_type_raw = 0x80);
    virtual bool set_configuration(uint8_t config_num);

    /// @brief Issue a control endpoint request to the device
    ///
    /// This function will block until completion. The names of the parameters of this function have the same meaning
    /// as the terms in the USB specification, section "Standard Device Requests".
    ///
    /// @param request_type bmRequestType - The type of the request. Pretty much a qualifier for `request`
    ///
    /// @param request The request actually being made. The standard requests are enumerated in the DEV_REQUEST
    ///                namespace.
    ///
    /// @param value Has a meaning specific to the request being made.
    ///
    /// @param index Has a meaning specific to the request being made.
    ///
    /// @param length Has a meaning specific to the request being made.
    ///
    /// @param buffer A buffer to store the results of the request in. Not all requests require a data buffer, but some
    ///               do. For those that do not, this parameter may equal nullptr. May also equal nullptr if length is
    ///               zero.
    ///
    /// @return True if the request was made successfully, false otherwise.
    virtual bool device_request(device_request_type request_type,
                                uint8_t request,
                                uint16_t value,
                                uint16_t index,
                                uint16_t length,
                                void *buffer) = 0;

    /// @brief Return the max packet size value in use for the default control endpoint.
    ///
    /// @return The current maximum packet size in bytes, or 0 if it is not known.
    virtual uint16_t get_max_packet_size() = 0;

    /// @brief Update the device's maximum packet size for the default control endpoint.
    ///
    /// This isn't known until after the device descriptor is read for the first time.
    ///
    /// @param new_packet_size The new maximum packet size to use.
    ///
    /// @return True if this operation completed successfully, false otherwise.
    virtual bool set_max_packet_size(uint16_t new_packet_size) = 0;

    /// @brief Configure the USB device using the specific configuration number given.
    ///
    /// @param config_num The number of the configuration to use. Must be less than
    ///                   main_device_descriptor.num_configurations. This parameters is the index into the array
    ///                   configurations, not the value sent over the wire to the device.
    ///
    /// @return True if the device entered the configured state successfully. False otherwise.
    virtual bool configure_device(uint8_t config_num) = 0;

    /// @brief Queue a transfer for the specified endpoint and direction.
    ///
    /// @param endpoint_num The endpoint to transfer on (valid range: 1-15). For control transfers, use
    ///                     device_request()
    ///
    /// @param is_inwards Is this an inwards transfer (or an outwards one)?
    ///
    /// @param transfer_item The transfer to execute.
    ///
    /// @return True if the transfer was queued successfully, false otherwise. DOES NOT indicate the success of the
    ///         transfer as a whole.
    virtual bool queue_transfer(uint8_t endpoint_num,
                                bool is_inwards,
                                std::shared_ptr<normal_transfer> transfer_item) = 0;

    //-------------------
    // Member variables.
    //-------------------
    device_descriptor main_device_descriptor; ///< Contains the contents of the USB device descriptor.
    std::unique_ptr<device_config[]> configurations; ///< Contains copies of all configurations for this device.

    uint8_t active_configuration; ///< Index in to configurations for the active configuration on this device.
  };

  /// A generic USB device.
  ///
  /// At present, this doesn't do a whole lot, but hopefully in future it will be able to handle most device-
  /// independent functionality.
  class generic_device : public IDevice
  {
  public:
    generic_device(std::shared_ptr<generic_core> core, uint16_t interface_num, const kl_string name);
    virtual ~generic_device() = default;

    // Overrides of IDevice
    virtual bool start() override { set_device_status(DEV_STATUS::OK); return true; };
    virtual bool stop() override { set_device_status(DEV_STATUS::STOPPED); return true; };
    virtual bool reset() override { set_device_status(DEV_STATUS::STOPPED); return true; };

    virtual void transfer_completed(normal_transfer *complete_transfer);

  protected:
    std::shared_ptr<generic_core> device_core; ///< The controller-specific core object this device drives.
    uint16_t device_interface_num; ///< The interface number to use with the core of multi-interface devices.
    device_interface &active_interface; ///< The interface in the device core that we're using.
  };

};