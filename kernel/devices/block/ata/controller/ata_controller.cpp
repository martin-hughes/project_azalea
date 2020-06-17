/// @file
/// @brief Implementation of parts of an ATA Host Controller that are not dependent on the controller's architecture.

//#define ENABLE_TRACING

#include "ata_controller.h"
#include "tracing.h"

using namespace ata;

namespace ata
{
/// A list of ATA commands that ata::generic_controller supports.
///
const command_properties known_commands[num_known_commands] = {
    { 0x20, true,  false, 0x100,   false, true,  false }, // READ
    { 0x24, true,  false, 0x10000, false, true,  true },  // READ_EXT
    { 0x25, true,  false, 0x10000, true,  true,  true },  // READ_EXT_DMA
    { 0x30, false, true,  0x100,   false, true,  false }, // WRITE
    { 0x34, false, true,  0x10000, false, true,  true },  // WRITE_EXT
    { 0x35, false, true,  0x10000, true,  true,  true },  // WRITE_EXT_DMA
    { 0xc8, true,  false, 0x100,   true,  true,  false }, // READ_DMA
    { 0xCA, false, true,  0x100,   true,  true,  false }, // WRITE_DMA
    { 0xEC, true,  false, 0x01,    false, false, false }, // IDENTIFY. Identify claims to read a sector because it
                                                          // retrieves a 512-byte block of information about the disk.
    { 0xE7, false, false, 0x00,    false, false, false }, // FLUSH CACHE
    { 0xEA, false, false, 0x00,    false, false, false }, // FLUSH CACHE EXT
  };
} // Namespace ata

/// @brief Issue an IDENTIFY command, and copy the results into the provided buffer.
///
/// @param[out] identity Storage for the output of the identity command. If the command fails, this buffer is left
///                      unchanged.
///
/// @param drive_index Which drive should have the command sent to it.
///
/// @return True if the command succeeded, false otherwise.
bool generic_controller::cmd_identify(identify_cmd_output &identity, uint16_t drive_index)
{
  bool result;

  KL_TRC_ENTRY;

  result = issue_command(drive_index, COMMANDS::IDENTIFY, 0, 1, 0, &identity, sizeof(identity));

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}