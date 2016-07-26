// 64bit's ACPI interface. Primarily bumps stuff back and forth to ACPICA.

#include "acpi_if.h"
#include "klib/klib.h"

extern "C"
{
#include "acpi/acpica/source/include/acpi.h"
}

void acpi_init_table_system()
{
  KL_TRC_ENTRY;

  if (AcpiInitializeSubsystem() != AE_OK)
  {
    panic("Failed to initialize ACPI");
  }

  if (AcpiLoadTables() != AE_OK)
  {
    panic("Failed to load ACPI tables");
  }

  KL_TRC_EXIT;
}
