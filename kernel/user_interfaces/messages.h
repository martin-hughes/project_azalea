/** @file
 *  @brief Message passing interface helpers
 */

#ifndef __USER_INTFACE_MESSAGES_H
#define __USER_INTFACE_MESSAGES_H

#include "./keyboard.h"

/* System-defined message IDs: */

const uint64_t SM_DEV_START = 0; /**< A device object is requested to start. */
const uint64_t SM_DEV_STOP = 1; /**< A device object is requested to stop. */
const uint64_t SM_DEV_RESET = 2; /**< A device object is requested to reset. */
const uint64_t SM_DEV_REGISTER = 3; /**< Device monitor is requested to register a new device. */
const uint64_t SM_MSG_SET_TARGET = 4; /**< An object is requested to update the object is sends other messages to. */
const uint64_t SM_KEYDOWN = 5; /**< Signals a key has been pressed. */
const uint64_t SM_KEYUP = 6; /**< Signals a key has been released. */
const uint64_t SM_PIPE_NEW_DATA = 7; /**< A pipe has generated new data. */
const uint64_t SM_USB_CREATE_DEVICE = 8; /**< A new USB device has been discovered and should be created. */
const uint64_t SM_SET_OPTIONS = 9; /**< Set options associated with the target */
const uint64_t SM_GET_OPTIONS = 10; /**< Return options associated with the target */

/* System message structures */

/**
 * @brief Message that is sent whenever a key is pressed or released.
 **/
struct keypress_msg
{
  /**
   * Which key has been pressed?
   */
  KEYS key_pressed;

  /**
   * Are any modifiers present on this key press?
   */
  special_keys modifiers;

  /**
   * If the key corresponds to a printable character, then it is set here. Otherwise, zero is given.
   */
  char printable;
};

#endif