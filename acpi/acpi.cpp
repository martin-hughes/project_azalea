// 64bit's ACPI interface. Primarily bumps stuff back and forth to ACPICA.

#define ENABLE_TRACING

#include "klib/klib.h"
#include "acpi_if.h"

void acpi_init_table_system()
{
  KL_TRC_ENTRY;

  COMPILER_ASSERT(sizeof(unsigned int) == 4);

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
