/// @file
/// @brief Declare a VGA text-mode terminal.

#include "devices/generic/gen_vt.h"

namespace terms
{

/// @brief A terminal using a VGA text-mode display and standard keyboard for I/O.
///
class vga : public vt
{
public:
  vga(std::shared_ptr<IWritable> keyboard_pipe, void *display_area_virt);
  virtual ~vga() { };

  virtual void handle_private_msg(std::unique_ptr<msg::root_msg> &message) override;

  virtual void tmt_callback(tmt_msg_t m, TMT *vt, const void *a) override;

  virtual void handle_keyboard_msg(uint64_t msg_id, keypress_msg &key_msg);

protected:
  virtual void enable_cursor() override;
  virtual void disable_cursor() override;
  virtual void set_cursor_pos(uint8_t x, uint8_t y) override;

  static char translate_colour(tmt_color_t colour, bool bright);

  unsigned char *display_ptr; ///< Pointer to the text mode display in RAM.
  const uint16_t bytes_per_char = 2; ///< How many bytes-per-character are in memory?
};

/// @brief Structure for holding entries in a map from KEYS values to ANSI terminal key sequences.
struct vga_term_keymap_entry
{
  uint8_t num_chars; ///< How many characters are in this sequence.
  const char *char_ptr; ///< Pointer to an array of those characters.
};

extern vga_term_keymap_entry vga_keymap[];

}; // namespace terms.
