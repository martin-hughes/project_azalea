/// @file
/// @brief Declare a VGA text-mode terminal.

#include "devices/generic/gen_terminal.h"

namespace terms
{

/// @brief A terminal using a serial port for I/O.
///
class serial : public generic
{
public:
  serial(std::shared_ptr<IWritable> keyboard_pipe);
  virtual ~serial() { };

  virtual void write_raw_string(const char *out_string, uint16_t num_chars) override;

  virtual void temp_read_port();
};

}; // namespace terms.
