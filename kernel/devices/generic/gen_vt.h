/// @file
/// @brief Declare a virtual terminal.

#include "gen_terminal.h"

extern "C"
{
#include "external/libtmt/tmt.h"
}

namespace terms
{

/// @brief A terminal where the terminal contents are stored in memory on this device.
///
class vt : public generic
{
public:
  vt(std::shared_ptr<IWriteImmediate> keyboard_pipe);
  vt(std::shared_ptr<IWriteImmediate> keyboard_pipe, std::string root_name);
  virtual ~vt();

  // Overrides from generic_terminal
  virtual bool start() override;
  virtual bool stop() override;
  virtual bool reset() override;

  virtual void tmt_callback(tmt_msg_t m, TMT *vt, const void *a);

  virtual void write_raw_string(const char *out_string, uint16_t num_chars) override;

protected:
  /// @brief Display the cursor on screen.
  ///
  virtual void enable_cursor() = 0;

  /// @brief Hide the cursor from the screen.
  ///
  virtual void disable_cursor() = 0;

  /// @brief Set the cursor position on screen.
  ///
  /// @param x The x position to move the cursor to.
  ///
  /// @param y The y position to move the cursor to.
  virtual void set_cursor_pos(uint8_t x, uint8_t y) = 0;

  /// @brief Translate a colour code used by TMT into a VGA colour
  ///
  /// @param colour The colour code used internally by TMT.
  ///
  /// @param bright Do we want a bright version of this colour?
  ///
  /// @return A code between 0 and 15 representing the colour in VGA terms.
  static char translate_colour(tmt_color_t colour, bool bright);

  TMT *inner_vt; ///< A virtual terminal provided by libtmt.
};

}; // namespace terms.
