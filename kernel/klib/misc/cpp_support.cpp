/// @file
/// @brief Functions required to enable C++ support

#include "klib/klib.h"

extern "C" void __cxa_pure_virtual();

void __cxa_pure_virtual()
{
  panic("Pure virtual function call");
}

void std::__throw_length_error(char const *str)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Length error thrown: ", str, "\n");
  panic("C++ length error thrown");

  KL_TRC_EXIT;

  while(1) { }
}