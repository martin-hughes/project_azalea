/// @file
/// @brief Controls the system's terminals.
///
/// At present, the various components of a terminal don't talk to each other very well, so this file contains
/// functions that basically marshal data backwards and forwards. It really is intended to be temporary - hence the
/// name.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "gen_terminal.h"
#include "devices/terminals/vga_terminal.h"
#include "devices/terminals/serial_terminal.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/pipe/pipe_fs.h"

extern bool wait_for_term;

/// @brief A simple text based terminal outputting on the main display.
///
void simple_terminal()
{
  KL_TRC_ENTRY;

  std::shared_ptr<ISystemTreeLeaf> leaf;
  std::shared_ptr<IReadable> reader;
  std::shared_ptr<IWritable> stdin_writer;
  /*std::shared_ptr<IReadable> serial_reader;
  std::shared_ptr<IWritable> serial_writer;*/
  const uint64_t buffer_size = 10;
  char buffer[buffer_size];
  uint64_t bytes_read;
  klib_message_hdr msg_header;
  unsigned char *display_ptr;

  // VGA console version:

  // Set up the output pipe - the one that correlates to stdout/stderr.
  std::shared_ptr<ISystemTreeBranch> pipes_br = std::make_shared<system_tree_simple_branch>();
  ASSERT(pipes_br != nullptr);
  ASSERT(system_tree() != nullptr);
  ASSERT(system_tree()->add_child("pipes", pipes_br) == ERR_CODE::NO_ERROR);
  ASSERT(pipes_br->add_child("terminal-output", pipe_branch::create()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\terminal-output\\read", leaf) == ERR_CODE::NO_ERROR);
  reader = std::dynamic_pointer_cast<IReadable>(leaf);
  ASSERT(reader != nullptr);

  // Set up an input pipe (which maps to stdin)
  ASSERT(pipes_br->add_child("terminal-input", pipe_branch::create()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\terminal-input\\write", leaf) == ERR_CODE::NO_ERROR);
  stdin_writer = std::dynamic_pointer_cast<IWritable>(leaf);
  ASSERT(stdin_writer != nullptr);

  // Serial port version:

  /*
  // Set up the output pipe - the one that correlates to stdout/stderr.
  ASSERT(pipes_br->add_child("serial-output", pipe_branch::create()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\serial-output\\read", leaf) == ERR_CODE::NO_ERROR);
  serial_reader = std::dynamic_pointer_cast<IReadable>(leaf);
  ASSERT(serial_reader != nullptr);

  // Set up an input pipe (which maps to stdin)
  ASSERT(pipes_br->add_child("serial-input", pipe_branch::create()) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->get_child("pipes\\serial-input\\write", leaf) == ERR_CODE::NO_ERROR);
  serial_writer = std::dynamic_pointer_cast<IWritable>(leaf);
  ASSERT(serial_writer != nullptr);
  */


  display_ptr = reinterpret_cast<unsigned char *>(mem_allocate_virtual_range(1));
  mem_map_range(nullptr, display_ptr, 1);
  display_ptr += 0xB8000;

  terms::vga output_term(stdin_writer, display_ptr);
  //terms::serial serial_term(serial_writer);

  msg_register_process(task_get_cur_thread()->parent_process.get());

  wait_for_term = false;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Beginning terminal\n");

  while(1)
  {
    // Write any pending output data.
    if (reader->read_bytes(0, buffer_size, reinterpret_cast<uint8_t *>(buffer), buffer_size, bytes_read) ==
        ERR_CODE::NO_ERROR)
    {
      if (bytes_read != 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Write output\n");
        output_term.write_string(buffer, bytes_read);
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read\n");
    }
    /*
    if (serial_reader->read_bytes(0, buffer_size, reinterpret_cast<uint8_t *>(buffer), buffer_size, bytes_read) ==
        ERR_CODE::NO_ERROR)
    {
      if (bytes_read != 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Write output\n");
        serial_term.write_string(buffer, bytes_read);
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to read\n");
    }*/

    // Grab any keyboard messages, and send them to the stdin pipe.
    if (msg_retrieve_next_msg(msg_header) == ERR_CODE::NO_ERROR)
    {
      //KL_TRC_TRACE(TRC_LVL::FLOW, "Got keyboard message. ");
      switch (msg_header.msg_id)
      {
      case SM_KEYDOWN:
      case SM_KEYUP:
        KL_TRC_TRACE(TRC_LVL::FLOW, "Keyboard message\n");
        output_term.handle_keyboard_msg(msg_header);
        break;

      default:
        break;
      }
      //KL_TRC_TRACE(TRC_LVL::FLOW, "\n");

      msg_msg_complete(msg_header);
    }

    // Trigger the serial terminal to check for data
    //serial_term.temp_read_port();
  }

  KL_TRC_EXIT;
}
