/// @file
/// @brief ACPI Device enumeration and control
///
// Known defects:
// - time_register_clock_source should be subsumed by the dev_monitor system.

//#define ENABLE_TRACING

#include <string>

#include "kernel_all.h"

#include "../devices/pci/pci_int_link_device.h"
#include "../devices/legacy/rtc/rtc.h"
#include "../devices/legacy/serial/serial.h"
#include "device_monitor.h"

namespace
{
  ACPI_STATUS acpi_create_device_handler (ACPI_HANDLE ObjHandle,
                                          UINT32 Level,
                                          void *Context,
                                          void **Retval);

  void acpi_create_one_device(const char *dev_path, ACPI_HANDLE obj_handle, ACPI_DEVICE_INFO &dev_info);
};

/// @brief Enumerate the ACPI device namespace and create drivers for any devices that we find.
///
/// We ignore devices that are created by another detection mechanism.
void acpi_create_devices()
{
  KL_TRC_ENTRY;

  AcpiWalkNamespace (ACPI_TYPE_DEVICE,
                     ACPI_ROOT_OBJECT,
                     0xFFFFFFFF,
                     acpi_create_device_handler,
                     nullptr,
                     NULL,
                     NULL);

  KL_TRC_EXIT;
}

namespace
{

/// @brief Called by acpi_create_devices for each device in the ACPI namespace
///
/// Creates a device driver for those objects, if appropriate.
///
/// @param ObjHandle Handle of the Object to be used with other Acpi functions.
///
/// @param Level Level within the tree.
///
/// @param Context User-provided context, always Null in Azalea.
///
/// @param Retval Storage space for a return value, not used.
///
/// @return AE_OK, to continue enumeration.
ACPI_STATUS acpi_create_device_handler (ACPI_HANDLE ObjHandle,
                                        UINT32 Level,
                                        void *Context,
                                        void **Retval)
{
  ACPI_STATUS status;
  ACPI_DEVICE_INFO *dev_info;
  ACPI_BUFFER dev_path;

  dev_path.Length = 256;
  dev_path.Pointer = new char[256];

  /* Get the full path of this device and print it */
  status = AcpiGetName (ObjHandle, ACPI_FULL_PATHNAME, &dev_path);
  if (ACPI_SUCCESS (status))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Path: ", (const char *)dev_path.Pointer, "\n");

    status = AcpiGetObjectInfo (ObjHandle, &dev_info);
    if (ACPI_SUCCESS (status))
    {
      acpi_create_one_device(reinterpret_cast<const char *>(dev_path.Pointer), ObjHandle, *dev_info);
      ACPI_FREE(dev_info);
    }
  }
  return AE_OK;
}

/// @cond
#define IS_DEV_HID(y) \
  (strncmp(dev_info.HardwareId.String, \
           (y), \
           std::min(static_cast<uint16_t>(dev_info.HardwareId.Length), static_cast<uint16_t>(sizeof(y)))) == 0)
/// @endcond

/// @brief Create a single device driver for a device that has been enumerated.
///
/// @param dev_path Path to the device in ACPI.
///
/// @param obj_handle Handle the the device in ACPI. This handle will remain valid forever.
///
/// @param dev_info ACPI device information for the device. This object is deleted after this function completes.
void acpi_create_one_device(const char *dev_path, ACPI_HANDLE obj_handle, ACPI_DEVICE_INFO &dev_info)
{
  std::string pathname;
  std::shared_ptr<IDevice> empty;

  KL_TRC_ENTRY;

  pathname = dev_path;

  if (dev_info.Valid & ACPI_VALID_HID)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Valid HID: ", (const char *)dev_info.HardwareId.String, "\n");

    if (IS_DEV_HID("PNP0C0F"))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "PCI IRQ Link pseudo-device\n");

      // Drop the results of this, the driver is owned by the PCI system.
      pci_irq_link_device::create(pathname, obj_handle);
    }
    else if (IS_DEV_HID("PNP0B00"))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "RTC\n");
      std::shared_ptr<timing::rtc> clock = timing::rtc::create(obj_handle);
      time_register_clock_source(clock);
    }
    else if (IS_DEV_HID("PNP0501"))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "16550A-compatible serial port\n");
      std::shared_ptr<serial_port> s_port;
      dev::create_new_device(s_port, empty, obj_handle);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown device HID: ", (const char *)dev_info.HardwareId.String, "\n");
    }

  }


  KL_TRC_EXIT;
}

} // Local namespace
