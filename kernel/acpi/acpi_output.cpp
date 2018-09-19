// Overrides for ACPICA's output functions. The default output functions simply dump everything to printf, which we
// don't want.

//#define ENABLE_TRACING

#define EXPORT_ACPI_INTERFACES

extern "C"
{
#include "acpi.h"
#include "accommon.h"
}

#include <stdint.h>
#include "klib/panic/panic.h"
#include "klib/c_helpers/printf_fns.h"
#include "klib/tracing/tracing.h"

/*******************************************************************************
 *
 * FUNCTION:    AcpiError
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Error" message with module/line/version info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE AcpiError(const char *ModuleName, UINT32 LineNumber, const char *Format, ...)
{
  KL_TRC_ENTRY;
  va_list ArgList;
  char format_buf[128];
  char msg_buf[256];

  klib_snprintf(format_buf, 127, "%s %s", ACPI_MSG_ERROR, Format);

  va_start(ArgList, Format);
  klib_vsnprintf(msg_buf, 255, format_buf, ArgList);
  panic(msg_buf);
  va_end(ArgList);

  KL_TRC_EXIT;
}

ACPI_EXPORT_SYMBOL(AcpiError)

/*******************************************************************************
 *
 * FUNCTION:    AcpiException
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Status              - Status to be formatted
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Exception" message with module/line/version info
 *              and decoded ACPI_STATUS.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE AcpiException(const char *ModuleName, UINT32 LineNumber, ACPI_STATUS Status, const char *Format, ...)
{
  KL_TRC_ENTRY;
  va_list ArgList;

  ACPI_MSG_REDIRECT_BEGIN;

  /* For AE_OK, just print the message */

  if (ACPI_SUCCESS(Status))
  {
    AcpiOsPrintf(ACPI_MSG_EXCEPTION);

  }
  else
  {
    AcpiOsPrintf(ACPI_MSG_EXCEPTION "%s, ", AcpiFormatException(Status));
  }

  va_start(ArgList, Format);
  AcpiOsVprintf(Format, ArgList);
  ACPI_MSG_SUFFIX;
  va_end(ArgList);

  ACPI_MSG_REDIRECT_END;
  KL_TRC_EXIT;
}

ACPI_EXPORT_SYMBOL(AcpiException)

/*******************************************************************************
 *
 * FUNCTION:    AcpiWarning
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Warning" message with module/line/version info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE AcpiWarning(const char *ModuleName, UINT32 LineNumber, const char *Format, ...)
{
  KL_TRC_ENTRY;
  va_list ArgList;

  ACPI_MSG_REDIRECT_BEGIN;
  AcpiOsPrintf(ACPI_MSG_WARNING);

  va_start(ArgList, Format);
  AcpiOsVprintf(Format, ArgList);
  ACPI_MSG_SUFFIX;
  va_end(ArgList);

  ACPI_MSG_REDIRECT_END;
  KL_TRC_EXIT;
}

ACPI_EXPORT_SYMBOL(AcpiWarning)

/*******************************************************************************
 *
 * FUNCTION:    AcpiInfo
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print generic "ACPI:" information message. There is no
 *              module/line/version info in order to keep the message simple.
 *
 * TBD: ModuleName and LineNumber args are not needed, should be removed.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE AcpiInfo(const char *Format, ...)
{
  KL_TRC_ENTRY;
  va_list ArgList;
  char msg_buf[256];

  KL_TRC_TRACE(TRC_LVL::FLOW, ACPI_MSG_INFO);

  va_start(ArgList, Format);
  klib_vsnprintf(msg_buf, 255, Format, ArgList);
  KL_TRC_TRACE(TRC_LVL::FLOW, msg_buf, "\n");
  va_end(ArgList);

  KL_TRC_EXIT;
}

ACPI_EXPORT_SYMBOL(AcpiInfo)

/*******************************************************************************
 *
 * FUNCTION:    AcpiBiosError
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Firmware Error" message with module/line/version
 *              info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE AcpiBiosError(const char *ModuleName, UINT32 LineNumber, const char *Format, ...)
{
  KL_TRC_ENTRY;
  va_list ArgList;

  AcpiOsPrintf(ACPI_MSG_BIOS_ERROR);

  va_start(ArgList, Format);
  AcpiOsVprintf(Format, ArgList);
  ACPI_MSG_SUFFIX;
  va_end(ArgList);

  KL_TRC_EXIT;
}

ACPI_EXPORT_SYMBOL(AcpiBiosError)

/*******************************************************************************
 *
 * FUNCTION:    AcpiBiosWarning
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Firmware Warning" message with module/line/version
 *              info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE AcpiBiosWarning(const char *ModuleName, UINT32 LineNumber, const char *Format, ...)
{
  KL_TRC_ENTRY;

  va_list ArgList;
  char out_buf[256];

  KL_TRC_TRACE(TRC_LVL::FLOW, ACPI_MSG_BIOS_WARNING);

  va_start(ArgList, Format);
  klib_vsnprintf(out_buf, 255, Format, ArgList);
  KL_TRC_TRACE(TRC_LVL::FLOW, out_buf, "\n");


  //ACPI_MSG_SUFFIX:
  klib_snprintf(out_buf, 255, " (%8.8X/%s-%u)\n", ACPI_CA_VERSION, ModuleName, LineNumber);
  KL_TRC_TRACE(TRC_LVL::FLOW, out_buf, "\n");
  va_end(ArgList);


  KL_TRC_EXIT;
}

ACPI_EXPORT_SYMBOL (AcpiBiosWarning)
