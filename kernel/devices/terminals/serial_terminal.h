/// @file
/// @brief Declare a VGA text-mode terminal.

#include "../generic/gen_terminal.h"

namespace terms
{

/// @brief A terminal using a serial port for I/O.
///
class serial : public generic
{
public:
  serial(std::shared_ptr<IWritable> keyboard_pipe,
         std::shared_ptr<IWritable> output_port_s,
         std::shared_ptr<IReadable> input_port_s);
  virtual ~serial() { };

  // Overrides from terms::generic
  virtual void handle_pipe_new_data(std::unique_ptr<msg::root_msg> &message) override;
  virtual void write_raw_string(const char *out_string, uint16_t num_chars) override;

protected:
  std::shared_ptr<IWritable> output_port; ///< Where to write data to send it towards the terminal.
  std::shared_ptr<IReadable> input_port; ///< Where to read data from when it is sent by the terminal.
};

}; // namespace terms.
