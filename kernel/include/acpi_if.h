/// @file
/// @brief Interface to Azalea's ACPI functions.
///
/// Does not include the interface to ACPICA, which is separate.

#pragma once

// Ordinarily, this file would be called acpi.h, but this conflicts with a file in the ACPICA.

#include <stdint.h>

#ifndef DOXYGEN_BUILD
extern "C"
{
#include "acpi.h"
}
#endif

void acpi_init_table_system();
void acpi_finish_init();
void acpi_create_devices();

acpi_subtable_header *acpi_init_subtable_ptr(void *start_of_table, uint64_t offset);
acpi_subtable_header *acpi_advance_subtable_ptr(acpi_subtable_header *header);
