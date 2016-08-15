// 64bit's ACPI interface. Primarily bumps stuff back and forth to ACPICA.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "acpi_if.h"

void acpi_init_table_system()
{
  KL_TRC_ENTRY;

  static_assert(sizeof(unsigned int) == 4, "Unsigned long assumed to be length 4");

  if (AcpiInitializeSubsystem() != AE_OK)
  {
    panic("Failed to initialize ACPI");
  }

  KL_TRC_TRACE((TRC_LVL_IMPORTANT, "ACPI Subsystem initialized\n"));

  if (AcpiInitializeTables((ACPI_TABLE_DESC*) NULL, 0, FALSE) != AE_OK)
  {
    panic("Failed to initialize ACPI tables");
  }

  KL_TRC_TRACE((TRC_LVL_IMPORTANT, "ACPI Tables initialized\n"));

  if (AcpiLoadTables() != AE_OK)
  {
    panic("Failed to load ACPI tables");
  }

  KL_TRC_TRACE((TRC_LVL_IMPORTANT, "ACPI Tables loaded\n"));

  KL_TRC_EXIT;
}

// Some helper functions for dealing with the subtable feature of ACPI.

acpi_subtable_header *acpi_init_subtable_ptr(void *start_of_table, unsigned long offset)
{
  KL_TRC_ENTRY;

  unsigned long start = (unsigned long)start_of_table;
  unsigned long result = start + offset;

  KL_TRC_DATA("Start of table", start);
  KL_TRC_DATA("Offset", offset);
  KL_TRC_DATA("Result", result);

  KL_TRC_EXIT;

  return (acpi_subtable_header *)result;
}

acpi_subtable_header *acpi_advance_subtable_ptr(acpi_subtable_header *header)
{
  return acpi_init_subtable_ptr((void *)header, (unsigned long)header->Length);
}
