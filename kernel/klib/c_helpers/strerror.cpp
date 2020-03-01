/// @file
/// @brief Define the worst possible strerror() function.

#include <string.h>

/// @cond
char *strerror(int errnum)
{
  return nullptr;
}
/// @endcond
