// Project Azalea's ACPI interface. Primarily bumps stuff back and forth to ACPICA.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "acpi_if.h"

void acpi_init_table_system()
{
  KL_TRC_ENTRY;

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

// Some helper functions for dealing with the subtable feature of ACPI.

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

acpi_subtable_header *acpi_advance_subtable_ptr(acpi_subtable_header *header)
{
  return acpi_init_subtable_ptr((void *)header, (uint64_t)header->Length);
}
