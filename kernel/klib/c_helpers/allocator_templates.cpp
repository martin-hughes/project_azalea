/// @file
/// @brief Instantiate two required allocator templates.

#include <memory>

namespace std
{
  template class allocator<char>;
  template class allocator<wchar_t>;
}