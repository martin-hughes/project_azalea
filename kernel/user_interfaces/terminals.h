/** @file
 *  @brief Main Azalea kernel system call interface
 */

#ifndef __AZALEA_TERMINALS_USER_H
#define __AZALEA_TERMINALS_USER_H

#include "macros.h"

/** @brief Enum defining newline translations to be carried out.
 */
enum AZALEA_ENUM_CLASS term_newline_mode_T
{
  NO_CHANGE = 0, ///< Do not change any newline character.
  CR_TO_CRLF = 1, ///< Translate CR characters into CRLF.
  LF_TO_CRLF = 2, ///< Translate LF characters into CRLF.
};

/**
 * @cond */
AZALEA_RENAME_ENUM(term_newline_mode);
/**
 * @endcond */

/** @brief Structure defining filtering options for a terminal.
 *
 * These options are (or will be) analogous to Linux's stty options.
 *
 * Input filters refer to data flowing from terminal to system (e.g. keyboard key presses). Output refers to data
 * going to the terminal (e.g. stdout).
 */
struct terminal_opts
{
  /* Input filters.*/

  /** @brief Should a \\r character be interpreted as a \\n?
   *
   * Azalea uses \\n to delimit new lines, many terminals use \\r.
   */
  bool input_return_is_newline{true};

  /** @brief Is line discipline enabled?
   *
   * Unlike Linux, Azalea only supports two modes - fully enabled and relevant keys translated, or off.
   */
  bool line_discipline{true};

  /** @brief Should character 127 be treated as a backspace?
   *
   * If set to false, this character is ignored in line discipline mode.
   */
  bool char_7f_is_backspace{true};

  /* Output filters. */

  /** @brief How to translate newline characters being sent to screen.
   *
   * Default is newline_mode::CR_TO_CRLF.
   */
  term_newline_mode output_newline{term_newline_mode::LF_TO_CRLF};
};

#endif
