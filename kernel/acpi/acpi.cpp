/// @file
/// @brief Project Azalea's ACPI interface. Primarily bumps stuff back and forth to ACPICA.
//
// Known defects:
// - We make the assumption of only one PCI root bridge in the system. In the future, we will do proper ACPI detection
//   of devices, then this assumption can be revisited.

//#define ENABLE_TRACING

#include "klib/klib.h"

#ifndef DOXYGEN_BUILD
#include "acpi_if.h"
#endif

/// @brief Phase 1 of initialising the ACPI system.
///
/// This phase will allow access to the ACPI tables, but not any of the dynamic functionality.
///
/// If ACPI initialisation fails, this function will panic.
void acpi_init_table_system()
{
  KL_TRC_ENTRY;

  AcpiGbl_EnableInterpreterSlack = true;
  AcpiGbl_CopyDsdtLocally = true;
  AcpiGbl_UseDefaultRegisterWidths = true;
  AcpiGbl_EnableAmlDebugObject = false;
  AcpiGbl_TruncateIoAddresses = true;

  if (AcpiInitializeSubsystem() != AE_OK)
  {
    panic("Failed to initialize ACPI");
  }

  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "ACPI Subsystem initialized\n");

  if (AcpiInitializeTables((ACPI_TABLE_DESC*) nullptr, 0, FALSE) != AE_OK)
  {
    panic("Failed to initialize ACPI tables");
  }

  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "ACPI Tables initialized\n");

  if (AcpiLoadTables() != AE_OK)
  {
    panic("Failed to load ACPI tables");
  }

  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "ACPI Tables loaded\n");

  KL_TRC_EXIT;
}

/// @brief Phase 2 of initialising the ACPI system.
///
/// This will allow access to all ACPI functionality.
///
/// This function should not be called until threading is enabled. If ACPI initialisation fails, this function will
/// simply panic.
void acpi_finish_init()
{
  ACPI_STATUS status;

  KL_TRC_ENTRY;

  // Bring the ACPI system up to full readiness.
  status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
  ASSERT(status == AE_OK);

  status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
  ASSERT(status == AE_OK);

  KL_TRC_EXIT;
}

// Some helper functions for dealing with the subtable feature of ACPI.

/// @brief Create a pointer to an ACPI subtable
///
/// @param start_of_table Pointer to the beginning of the main table
///
/// @param offset Number of bytes after start_of_table that the subtable begins.
///
/// @return Pointer to the subtable.
acpi_subtable_header *acpi_init_subtable_ptr(void *start_of_table, uint64_t offset)
{
  KL_TRC_ENTRY;

  uint64_t start = reinterpret_cast<uint64_t>(start_of_table);
  uint64_t result = start + offset;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Start of table", start, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Offset", offset, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result", result, "\n");

  KL_TRC_EXIT;

  return (acpi_subtable_header *)result;
}

/// @brief Get a pointer to the next subtable in a chain.
///
/// @param header The subtable to advance from
///
/// @return Pointer to the next subtable after header.
acpi_subtable_header *acpi_advance_subtable_ptr(acpi_subtable_header *header)
{
  return acpi_init_subtable_ptr((void *)header, (uint64_t)header->Length);
}
