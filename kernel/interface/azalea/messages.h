/** @file
 *  @brief Message passing interface helpers
 */

#ifndef __USER_INTFACE_MESSAGES_H
#define __USER_INTFACE_MESSAGES_H

#include "./keyboard.h"

/* System-defined message IDs: */

static const uint64_t SM_DEV_START = 0; /**< A device object is requested to start. */
static const uint64_t SM_DEV_STOP = 1; /**< A device object is requested to stop. */
static const uint64_t SM_DEV_RESET = 2; /**< A device object is requested to reset. */
static const uint64_t SM_DEV_REGISTER = 3; /**< Device monitor is requested to register a new device. */
static const uint64_t SM_MSG_SET_TARGET = 4; /**< An object is requested to update the object is sends other messages to. */
static const uint64_t SM_KEYDOWN = 5; /**< Signals a key has been pressed. */
static const uint64_t SM_KEYUP = 6; /**< Signals a key has been released. */
static const uint64_t SM_PIPE_NEW_DATA = 7; /**< A pipe has generated new data. */
static const uint64_t SM_USB_CREATE_DEVICE = 8; /**< A new USB device has been discovered and should be created. */
static const uint64_t SM_SET_OPTIONS = 9; /**< Set options associated with the target */
static const uint64_t SM_GET_OPTIONS = 10; /**< Return options associated with the target */
static const uint64_t SM_USB_TRANSFER_COMPLETE = 11; /**< Requested USB transfer is complete */
static const uint64_t SM_XHCI_CMD_COMPLETE = 12; /**< XHCI controller command complete */
static const uint64_t SM_IO_MSG = 13; /**< Standard IO request message */
static const uint64_t SM_IO_COMPLETE = 14; /**< IO Request complete message */
static const uint64_t SM_ATA_CMD = 15; /**< ATA controller queued command */
static const uint64_t SM_ATA_CMD_COMPLETE = 16; /**< ATA controller queued command response */

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
  struct special_keys modifiers;

  /**
   * If the key corresponds to a printable character, then it is set here. Otherwise, zero is given.
   */
  char printable;
};

#endif