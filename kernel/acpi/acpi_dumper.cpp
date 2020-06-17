/// @file
/// @brief ACPI Table Dumper modified from the ACPICA user mode utility.
///
/// This copy and modifications are permitted by the BSD-style licence allowed by ACPICA.
///
/// Little of the contents of this file is documented further.
//
// This is a very rough-and-ready modification, don't expect high code quality here.

// @cond

#include "mem.h"

extern "C"
{
#include "acpi.h"
#include "accommon.h"
#include "actables.h"
}

#ifdef AZ_ACPI_DUMP_TABLES

#define AP_MAX_ACPI_TABLES 256

namespace
{

void AcpiUtDumpBufferToScreen (
    UINT8                   *Buffer,
    UINT32                  Count,
    UINT32                  Display,
    UINT32                  BaseOffset)
{
  UINT32                  i = 0;
  UINT32                  j;
  UINT32                  Temp32;
  UINT8                   BufChar;


  if (!Buffer)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Null Buffer Pointer in DumpBuffer!\n");
    return;
  }

  if ((Count < 4) || (Count & 0x01))
  {
    Display = DB_BYTE_DISPLAY;
  }

  /* Nasty little dump buffer routine! */

  while (i < Count)
  {
    /* Print current offset */

    AcpiOsPrintf("%8.4X: ", (BaseOffset + i));

    /* Print 16 hex chars */

    for (j = 0; j < 16;)
    {
      if (i + j >= Count)
      {
        /* Dump fill spaces */

        AcpiOsPrintf ("%*s", ((Display * 2) + 1), " ");
        j += Display;
        continue;
      }

      switch (Display)
      {
      case DB_BYTE_DISPLAY:
      default:    /* Default is BYTE display */

        AcpiOsPrintf("%02X ", Buffer[(ACPI_SIZE) i + j]);
        break;

      case DB_WORD_DISPLAY:

        ACPI_MOVE_16_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j]);
        AcpiOsPrintf("%04X ", Temp32);
        break;

      case DB_DWORD_DISPLAY:

        ACPI_MOVE_32_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j]);
        AcpiOsPrintf("%08X ", Temp32);
        break;

      case DB_QWORD_DISPLAY:

        ACPI_MOVE_32_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j]);
        AcpiOsPrintf("%08X", Temp32);

        ACPI_MOVE_32_TO_32 (&Temp32, &Buffer[(ACPI_SIZE) i + j + 4]);
        AcpiOsPrintf("%08X ", Temp32);
        break;
      }

      j += Display;
    }

    /*
      * Print the ASCII equivalent characters but watch out for the bad
      * unprintable ones (printable chars are 0x20 through 0x7E)
      */
    AcpiOsPrintf(" ");
    for (j = 0; j < 16; j++)
    {
      if (i + j >= Count)
      {
        AcpiOsPrintf("\n");
        return;
      }

      BufChar = Buffer[(ACPI_SIZE) i + j];
      if (isprint (BufChar))
      {
        AcpiOsPrintf("%c", BufChar);
      }
      else
      {
        AcpiOsPrintf(".");
      }
    }

    /* Done with that line. */

    AcpiOsPrintf( "\n");
    i += 16;
  }

  return;
}


BOOLEAN
ApIsValidHeader (
    ACPI_TABLE_HEADER       *Table)
{

  if (!ACPI_VALIDATE_RSDP_SIG (Table->Signature))
  {
    /* Make sure signature is all ASCII and a valid ACPI name */

    if (!AcpiUtValidNameseg (Table->Signature))
    {
      AcpiOsPrintf("Table signature (0x%8.8X) is invalid\n",
          *(UINT32 *) Table->Signature);
      return (FALSE);
    }

    /* Check for minimum table length */

    if (Table->Length < sizeof (ACPI_TABLE_HEADER))
    {
      AcpiOsPrintf("Table length (0x%8.8X) is invalid\n",
          Table->Length);
      return (FALSE);
    }
  }

  return (TRUE);
}

UINT32
ApGetTableLength (
    ACPI_TABLE_HEADER       *Table)
{
    ACPI_TABLE_RSDP         *Rsdp;


    /* Check if table is valid */

    if (!ApIsValidHeader (Table))
    {
        return (0);
    }

    if (ACPI_VALIDATE_RSDP_SIG (Table->Signature))
    {
        Rsdp = ACPI_CAST_PTR (ACPI_TABLE_RSDP, Table);
        return (AcpiTbGetRsdpLength (Rsdp));
    }

    /* Normal ACPI table */

    return (Table->Length);
}

static int
ApDumpTableBuffer (
    ACPI_TABLE_HEADER       *Table,
    UINT32                  Instance,
    ACPI_PHYSICAL_ADDRESS   Address)
{
    UINT32                  TableLength;


    TableLength = ApGetTableLength (Table);
    char buf[80];
    memset(buf, 0, 80);

    /* Print only the header if requested */

#if 0
    if (Gbl_SummaryMode)
    {
        AcpiTbPrintTableHeader (Address, Table);
        return (0);
    }

    /* Dump to binary file if requested */

    if (Gbl_BinaryMode)
    {
        return (ApWriteToBinaryFile (Table, Instance));
    }
#endif

    /*
     * Dump the table with header for use with acpixtract utility.
     * Note: simplest to just always emit a 64-bit address. AcpiXtract
     * utility can handle this.
     */
    AcpiOsPrintf("%4.4s @ 0x%8.8X%8.8X\n",
        Table->Signature, ACPI_FORMAT_UINT64 (Address));

    AcpiUtDumpBufferToScreen (
        ACPI_CAST_PTR (UINT8, Table), TableLength,
        DB_BYTE_DISPLAY, 0);
    AcpiOsPrintf("\n");
    return (0);
}

} // Local namespace

/// @endcond

/// @brief Dump all ACPI tables to the debug output
///
void acpi_dump_all_tables (void)
{
  ACPI_TABLE_HEADER       *Table;
  UINT32                  Instance = 0;
  ACPI_PHYSICAL_ADDRESS   Address;
  ACPI_STATUS             Status;
  int                     TableStatus;
  UINT32                  i;


  /* Get and dump all available ACPI tables */

  for (i = 0; i < AP_MAX_ACPI_TABLES; i++)
  {
    Status = AcpiGetTableByIndex (i, &Table);
    Address = (uint64_t)mem_get_phys_addr(Table);
    if (ACPI_FAILURE (Status))
    {
      /* AE_LIMIT means that no more tables are available */

      if (Status == AE_LIMIT)
      {
        return;
      }
      else if (i == 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Could not get ACPI tables - error ",
                     AcpiFormatException (Status), "\n");
        return;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Could not get ACPI table at index ", i, " error ",
                     AcpiFormatException (Status), "\n");
        continue;
      }
    }

    TableStatus = ApDumpTableBuffer (Table, Instance, Address);

    if (TableStatus)
    {
      break;
    }
  }

  /* Something seriously bad happened if the loop terminates here */

  return;
}

#endif
