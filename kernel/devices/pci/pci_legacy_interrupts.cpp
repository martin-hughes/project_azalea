/// @file
/// @brief Implements management of the PCI legacy (non-MSI/MSI-X) interrupt connections.
//
// Known defects:
// - We quite often refer to 'interrupt' here but what we really mean is IRQ. In future, I'd like to be able to support
//   e.g. multiple APICs so interrupt number might be more helpful then.
// - This file is fundamentally single threaded. This is acceptable for the time being, because so is the ACPI
//   enumeration process that drives it.
// - The Bochs 'cheat mode' for choosing PCI IRQs ignores LNKS that is used for a power management device.

// This file starts of with an object to manage a single PCI device-IRQ link. It then has functions that initialise and
// manage a mapping of device address to IRQ connections.

//#define ENABLE_TRACING

#include <string>
#include <map>

#include "acpi/acpi_if.h"
#include "pci.h"
#include "pci_int_link_device.h"
#include "generic_device/pci_generic_device.h"
#include "klib/klib.h"

extern bool is_bochs_machine;

namespace
{
  /// @brief Stores calculated PCI device legacy IRQ numbers
  ///
  struct pci_device_interrupts
  {
    uint16_t pin_irq[4]; ///< IRQ for each pins.
  };

  /// @brief Wrapper around ACPI_PCI_ID to make it easier to use the raw form.
  ///
  union acpi_pci_addr
  {
    struct
    { // These names are spelled with upper case letters to match ACPICA.
      uint16_t Function; ///< Device function.
      uint16_t Device; ///< Device ID.
      uint16_t Bus; ///< Device bus.
      uint16_t Segment; ///< Device segment.
    } normal; ///< Address in encoded format.
    uint64_t raw; ///< Address in raw format.
  };
  static_assert(sizeof(ACPI_PCI_ID) == 8);
  static_assert(sizeof(acpi_pci_addr) == 8);

  // Function forward declarations:
  void pci_init_int_map();
  acpi_pci_addr acpi_addr_from_pci_addr(pci_address addr);

  /// @brief Stores the link devices so we do not have to constantly re-retrieve them from ACPI.
  ///
  /// The key is the path of the object in ACPI.
  ///
  /// The value is the device itself.
  std::map<std::string, std::shared_ptr<pci_irq_link_device>> *link_devices = nullptr;

  /// @brief Stores the mappings between PCI devices and the interrupts they are connected to.
  ///
  /// The key is the address of the PCI device, in ACPI _ADR format.
  ///
  /// The value is the connections the device is connected to.
  std::map<uint32_t, pci_device_interrupts> *pci_int_map = nullptr;

  /// @brief Order of preference of IRQs.
  ///
  /// The first IRQ number will be used by the first pci_irq_link_device to be created, and the second by the second,
  /// etc.
  ///
  /// An entry of -1 indicates that this entry should be skipped over.
  int32_t irq_preference_order[] = {10, 11, 3, 5, 6, 7};
  const uint16_t num_preferences = 6; ///< How many entries are in the above array.
}

/// @brief Construct a new pci_irq_link_device object and store it in our list of such devices.
///
/// @param pathname The fully qualified path name for this link device.
///
/// @param obj_handle Handle to the object.
///
/// @return Shared pointer to the new device.
std::shared_ptr<pci_irq_link_device> pci_irq_link_device::create(std::string &pathname, ACPI_HANDLE obj_handle)
{
  std::shared_ptr<pci_irq_link_device> new_device;

  KL_TRC_ENTRY;

  if (!link_devices)
  {
    link_devices = new std::map<std::string, std::shared_ptr<pci_irq_link_device>>;
  }

  new_device = std::shared_ptr<pci_irq_link_device>(new pci_irq_link_device(obj_handle));
  ASSERT(!map_contains(*link_devices, pathname));

  KL_TRC_TRACE(TRC_LVL::FLOW, "Added new device with pathname: ", pathname, "\n");

  link_devices->insert({pathname, new_device});

  KL_TRC_EXIT;

  return new_device;
}

/// @brief Construct the device and chose an interrupt for the link to use.
///
/// @param dev_handle ACPI handle to the link device in the ACPI namespace.
pci_irq_link_device::pci_irq_link_device(ACPI_HANDLE dev_handle) : chosen_interrupt{0}
{
  ACPI_STATUS status;
  ACPI_BUFFER possible_resources;
  uint8_t *raw_ptr;
  ACPI_RESOURCE *resource_ptr;
  bool found_irq = false;
  uint16_t match;

  KL_TRC_ENTRY;

  possible_resources.Length = ACPI_ALLOCATE_BUFFER;
  possible_resources.Pointer = nullptr;

  status = AcpiGetPossibleResources(dev_handle, &possible_resources);
  ASSERT(status == AE_OK); // Pretty harsh assert, but it'll help check assumptions for now.

  raw_ptr = reinterpret_cast<uint8_t *>(possible_resources.Pointer);
  resource_ptr = reinterpret_cast<ACPI_RESOURCE *>(raw_ptr);

  while ((resource_ptr->Length != 0) && (resource_ptr->Type != ACPI_RESOURCE_TYPE_END_TAG) && !found_irq)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Resource type: ", resource_ptr->Type, "\n");
    switch (resource_ptr->Type)
    {
    case ACPI_RESOURCE_TYPE_IRQ:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Normal IRQ list\n");
      match = chose_interrupt(resource_ptr->Data.Irq.InterruptCount, resource_ptr->Data.Irq.Interrupts, 1);
      if (match != 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found valid IRQ: ", match, "\n");
        chosen_interrupt = match;
        found_irq = true;
      }
      break;

    case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Extended IRQ list\n");
      match = chose_interrupt(resource_ptr->Data.ExtendedIrq.InterruptCount,
                              resource_ptr->Data.ExtendedIrq.Interrupts,
                              4);
      if (match != 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found valid IRQ: ", match, "\n");
        chosen_interrupt = match;
        found_irq = true;
      }
      break;

    default:
      panic("Unrecognised resource type");
    }

    raw_ptr += resource_ptr->Length;
    resource_ptr = reinterpret_cast<ACPI_RESOURCE *>(raw_ptr);
  }

  KL_TRC_EXIT;
}

/// @cond
#define GET_INTERRUPT_CHOICE(idx, size) \
  (bytes_per_int == 1) ? (reinterpret_cast<uint8_t *>(choices))[(idx)] : (reinterpret_cast<uint32_t *>(choices))[(idx)]
/// @endcond

/// @brief Given a list of possible interrupts, chose our favorite.
///
/// @param interrupt_count How many interrupts are in the 'choices' array.
///
/// @param choices The array of choices for us to look through.
///
/// @param bytes_per_int How many bytes long is each entry in choices.
///
/// @return The chosen interrupt. Zero indicates an error.
uint16_t pci_irq_link_device::chose_interrupt(uint16_t interrupt_count, void *choices, uint8_t bytes_per_int)
{
  uint16_t chosen = 0;
  uint16_t preferred_int_idx = 0;
  bool found_int = false;
  bool no_more_prefs = false;
  uint16_t possible;

  KL_TRC_ENTRY;

  ASSERT(choices != nullptr);
  ASSERT(interrupt_count > 0);
  ASSERT((bytes_per_int == 1) || (bytes_per_int == 4));

  // Loop until we find an interrupt to use. Start by looking through our list of preferences to get the next one,
  // skipping over the ones we've already used. Then look through the list of interrupts offered by the device to see
  // if one of them is our preference. If not, try the next preference, and so on.
  while(!found_int)
  {
    while(preferred_int_idx < num_preferences)
    {
      if (irq_preference_order[preferred_int_idx] != -1)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW,
                     "Go with index: ", preferred_int_idx,
                     " / preference ", irq_preference_order[preferred_int_idx],
                     "\n");
        break;
      }

      preferred_int_idx++;
    }

    if (preferred_int_idx >= num_preferences)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Run out of preferences\n");
      no_more_prefs = true;
    }

    if (no_more_prefs)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Just use the first IRQ offered\n");
      chosen = GET_INTERRUPT_CHOICE(0, bytes_per_int);
      found_int = true;
      break;
    }
    else
    {
      for (uint16_t possible_idx = 0; possible_idx < interrupt_count; possible_idx++)
      {
        possible = GET_INTERRUPT_CHOICE(possible_idx, bytes_per_int);
        KL_TRC_TRACE(TRC_LVL::FLOW, "What about IRQ ", possible, "?");

        if (possible == irq_preference_order[preferred_int_idx])
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, " YES\n");
          irq_preference_order[preferred_int_idx] = -1;
          chosen = possible;
          found_int = true;
          break;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, " NO.\n");
        }
      }

      // Didn't find our preference in the list of possible IRQs, so move to the next one.
      preferred_int_idx++;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", chosen, "\n");
  KL_TRC_EXIT;

  return chosen;
}

/// @brief Return the IRQ this device is attached to.
///
/// @return The interrupt this link device is attached to.
uint16_t pci_irq_link_device::get_interrupt()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return chosen_interrupt;
}

/// @brief Compute the actual interrupt number for a given device and pin
///
/// PCI devices define four interrupt pins (A-D), but these can be mapped by the PIC/APIC to any interrupt number at
/// the CPU, and this varies by device.
///
/// @param pin The pin on the device to evaluate. Valid values are 0-3, where 0 = pin A, 1 = pin B, etc.
///
/// @return The interrupt number that needs servicing. If the result is 0 then the lookup failed.
uint16_t pci_generic_device::compute_irq_for_pin(uint8_t pin)
{
  uint16_t result = 0;
  acpi_pci_addr address;

  KL_TRC_ENTRY;

  ASSERT(pin < 4);

  if (is_bochs_machine)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "This is a Bochs machine, so cheat\n");

    // First index is slot % 4, second is pin - 1.
    uint16_t lnk_a_int;
    uint16_t lnk_b_int;
    uint16_t lnk_c_int;
    uint16_t lnk_d_int;

    ASSERT(map_contains(*link_devices, "\\_SB_.LNKA"));
    ASSERT(map_contains(*link_devices, "\\_SB_.LNKB"));
    ASSERT(map_contains(*link_devices, "\\_SB_.LNKC"));
    ASSERT(map_contains(*link_devices, "\\_SB_.LNKD"));

    lnk_a_int = link_devices->find("\\_SB_.LNKA")->second->get_interrupt();
    lnk_b_int = link_devices->find("\\_SB_.LNKB")->second->get_interrupt();
    lnk_c_int = link_devices->find("\\_SB_.LNKC")->second->get_interrupt();
    lnk_d_int = link_devices->find("\\_SB_.LNKD")->second->get_interrupt();
    KL_TRC_TRACE(TRC_LVL::FLOW, "A: ", lnk_a_int, ", B:", lnk_b_int, ", C:", lnk_c_int, ", D:", lnk_d_int, "\n");

    uint16_t irq_table[4][4] = {
      { lnk_a_int, lnk_b_int, lnk_c_int, lnk_d_int },
      { lnk_b_int, lnk_c_int, lnk_d_int, lnk_a_int },
      { lnk_c_int, lnk_d_int, lnk_a_int, lnk_b_int },
      { lnk_d_int, lnk_a_int, lnk_b_int, lnk_c_int },
    };

    result = irq_table[address.normal.Device % 4][pin];
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not a Bochs machine, use longhand\n");

    if (pci_int_map == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieve PCI Legacy Interrupt map\n");
      pci_init_int_map();
    }

    address = acpi_addr_from_pci_addr(_address);
    address.normal.Function = 0xFFFF;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Lookup address: ", address.raw, "\n");
    if (map_contains(*pci_int_map, address.raw))
    {
      result = pci_int_map->find(address.raw)->second.pin_irq[pin];
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

namespace
{

/// @brief Retrieve and initialise the mapping between PCI IRQ lines and actual interrupts.
///
void pci_init_int_map()
{
  ACPI_STATUS status;
  ACPI_HANDLE root_dev;
  ACPI_BUFFER ret_buffer;
  ACPI_PCI_ROUTING_TABLE *pci_route;
  uint8_t *raw_ptr;
  ACPI_BUFFER irq_buffer;
  pci_device_interrupts int_table;
  std::string link_device_name;
  std::shared_ptr<pci_irq_link_device> link_dev;
  uint16_t pin_interrupt;

  KL_TRC_ENTRY;

  pci_int_map = new std::map<uint32_t, pci_device_interrupts>;

  status = AcpiGetHandle(NULL, ACPI_STRING("\\_SB_.PCI0"), &root_dev);
  ASSERT(status == AE_OK);

  ret_buffer.Pointer = nullptr;
  ret_buffer.Length = ACPI_ALLOCATE_BUFFER;
  irq_buffer.Pointer = nullptr;
  irq_buffer.Length = ACPI_ALLOCATE_BUFFER;

  status = AcpiGetIrqRoutingTable(root_dev, &ret_buffer);
  KL_TRC_TRACE(TRC_LVL::FLOW, "GetIrqRoutingTable result: ", status, "\n");
  ASSERT(status == AE_OK);

  raw_ptr = reinterpret_cast<uint8_t *>(ret_buffer.Pointer);
  pci_route = reinterpret_cast<ACPI_PCI_ROUTING_TABLE *>(raw_ptr);
  while (pci_route->Length != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW,
                 "- ", pci_route->Address,
                 " pin: ", pci_route->Pin,
                 ", ", pci_route->SourceIndex,
                 ", ", (const char *)pci_route->Source,
                 "\n");

    // Search for an existing device to update.
    if (map_contains(*pci_int_map, pci_route->Address))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieve existing details\n");
      int_table = pci_int_map->find(pci_route->Address)->second;
      pci_int_map->erase(pci_route->Address); // This is a bit awkward, but we dont's support in-place updates yet.
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Create new details\n");
      memset(&int_table, 0, sizeof(int_table));
    }

    // If there's a device name, use it. Otherwise, the source index is the IRQ number.
    if (pci_route->Source[0] != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Use device: ", (const char *)pci_route->Source);
      link_device_name = std::string(reinterpret_cast<const char *>(pci_route->Source));

      ASSERT(map_contains(*link_devices, link_device_name));
      pin_interrupt = link_devices->find(link_device_name)->second->get_interrupt();
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Use interrupt: ", pci_route->SourceIndex);
      pin_interrupt = pci_route->SourceIndex;
    }

    ASSERT(pci_route->Pin < 4);

    // Save details.
    KL_TRC_TRACE(TRC_LVL::FLOW, " to give interrupt: ", pin_interrupt, "\n");
    int_table.pin_irq[pci_route->Pin] = pin_interrupt;
    pci_int_map->insert({pci_route->Address, int_table});

    // Advance to next route info.
    raw_ptr += pci_route->Length;
    pci_route = reinterpret_cast<ACPI_PCI_ROUTING_TABLE *>(raw_ptr);
  }

  KL_TRC_EXIT;
}

/// @brief Convert a packed PCI address into an ACPI address.
acpi_pci_addr acpi_addr_from_pci_addr(pci_address addr)
{
  acpi_pci_addr result;

  KL_TRC_ENTRY;

  result.normal.Segment = 0; // No PCI-E segments in Azalea yet.
  result.normal.Bus = addr.bus;
  result.normal.Device = addr.device;
  result.normal.Function = addr.function;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result.raw, "\n");
  KL_TRC_EXIT;

  return result;
}

} // Local namespace.
