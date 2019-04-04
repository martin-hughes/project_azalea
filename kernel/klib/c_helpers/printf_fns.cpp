// Provides the kernel's internal implementations of snprintf and vsnprintf.
// No direct printf type function is exposed - the kernel doesn't output anything directly.

#include "klib/klib.h"

//#ifndef AZALEA_TEST_CODE
//#include "external/vsnprintf/vsnprintf.h"
//#else

// In the test code, do a bit of name-management to get the system vsnprintf to be used, rather than our imported
// external version that doesn't really place nicely.
#include <stdarg.h>
#include <stdio.h>
#include <cstdarg>
#include <cstdio>

using namespace std;

//#define rpl_vsnprintf vsnprintf

//#endif


uint32_t klib_snprintf(char *out_str, uint64_t max_out_len, const char *fmt, ...)
{
  KL_TRC_ENTRY;
  int retval;
  va_list args;

  va_start(args, fmt);

  retval = klib_vsnprintf(out_str, max_out_len, fmt, args);

  va_end(args);

  KL_TRC_EXIT;

  return retval;
}

uint32_t klib_vsnprintf(char *out_str, uint64_t max_out_len, const char *fmt, va_list args)
{
  int retval;
  KL_TRC_ENTRY;

  retval = vsnprintf(out_str, max_out_len, fmt, args);

  KL_TRC_EXIT;
  return retval;
}
