#ifndef ACPI_MAIN_HEADER
#define ACPI_MAIN_HEADER

// Ordinarily, this file would be called acpi.h, but this conflicts with a file in the ACPICA.

extern "C"
{
#include "acpi/acpica/source/include/acpi.h"
}

void acpi_init_table_system();

#endif
