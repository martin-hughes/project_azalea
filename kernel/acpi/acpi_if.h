#ifndef ACPI_MAIN_HEADER
#define ACPI_MAIN_HEADER

// Ordinarily, this file would be called acpi.h, but this conflicts with a file in the ACPICA.

#include <stdint.h>

extern "C"
{
#include "external/acpica/source/include/acpi.h"
}

void acpi_init_table_system();

acpi_subtable_header *acpi_init_subtable_ptr(void *start_of_table, uint64_t offset);
acpi_subtable_header *acpi_advance_subtable_ptr(acpi_subtable_header *header);

#endif
