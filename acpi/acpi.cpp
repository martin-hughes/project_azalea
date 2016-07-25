// 64bit's ACPI interface. Primarily bumps stuff back and forth to ACPICA.

#include "acpi/acpi.h"
#include "klib/klib.h"

extern "C"
{
#include "acpi/acpica/source/include/acpi.h"
}

void acpi_init_table_system()
{
  KL_TRC_ENTRY;

  AcpiLoadTables();

  KL_TRC_EXIT;
}
