/// @file
/// @brief Functions to appease the linker when compiling the test script.
///
/// These functions are not supported by the Azalea kernel, so anything relying on these features within Google Test
/// will fail.

#include <sys/stat.h>

extern "C"
{
int mkdir(const char *_, mode_t _2)
{
  return 0;
}

int mkstemp (char * _)
{
  return 0;
}

int dup(int)
{
  return 0;
}

int dup2(int, int)
{
  return 0;
}

}
