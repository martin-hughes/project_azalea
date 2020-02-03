/// @file
/// @brief Implementation of the per-process parts of a 'proc'-like filesystem.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/fs/proc/proc_fs.h"
#include "system_tree/fs/mem/mem_fs.h"

#include <string.h>
#include <stdio.h>

using namespace std;

/// @brief Default constructor
///
/// This object should be created using the static create() function.
///
/// @param related_proc Which process does this branch store details of?
proc_fs_root_branch::proc_fs_proc_branch::proc_fs_proc_branch(std::shared_ptr<task_process> related_proc) :
  _related_proc(related_proc), _id_file(new mem_fs_leaf(nullptr))
{
  KL_TRC_ENTRY;

  char id_buffer[22];
  ERR_CODE ec;
  uint64_t br;
  uint64_t strl;

  ASSERT(related_proc != nullptr);

  // Create a file containing the process ID number.
  ASSERT(_id_file != nullptr);

  snprintf(id_buffer, 22, "%p", related_proc.get());
  strl = strnlen(id_buffer, 22);

  ec = _id_file->write_bytes(0, strl + 1, reinterpret_cast<const uint8_t *>(id_buffer), 22, br);
  ASSERT(ec == ERR_CODE::NO_ERROR);
  ASSERT(br == strl + 1);
  ec = system_tree_simple_branch::add_child("id", _id_file);
  ASSERT(ec == ERR_CODE::NO_ERROR);

  KL_TRC_EXIT;
}

/// @brief Create a new proc_fs_proc_branch
///
/// @param related_proc Which process does this branch store details of?
///
/// @return shared_ptr to a new proc_fs_proc_branch.
std::shared_ptr<proc_fs_root_branch::proc_fs_proc_branch>
  proc_fs_root_branch::proc_fs_proc_branch::create(std::shared_ptr<task_process> related_proc)
{
  return std::shared_ptr<proc_fs_root_branch::proc_fs_proc_branch>(
    new proc_fs_root_branch::proc_fs_proc_branch(related_proc));
}

proc_fs_root_branch::proc_fs_proc_branch::~proc_fs_proc_branch()
{
  KL_TRC_ENTRY;

  system_tree_simple_branch::delete_child("id");

  KL_TRC_EXIT;
}
