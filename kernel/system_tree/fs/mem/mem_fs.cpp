/// @brief Implementation of an in-memory file system for Azalea.

// Known defects:
// - There's no maximum size anywhere to be found!
// - When writing in to the buffer we should check for overflows.
// - We resize the buffer in quite a dumb way,
// - Using the names _buffer_length and buffer_length isn't all that obvious!

//#define ENABLE_TRACING

#include <klib/klib.h>
#include "mem_fs.h"

#include <memory>

using namespace std;

/// @brief Standard constructor
///
/// Mem FS branches should be created using the 'create' static member.
mem_fs_branch::mem_fs_branch()
{

}

/// @brief Create a new Mem FS branch.
///
/// @return Shared pointer to a new branch.
std::shared_ptr<mem_fs_branch> mem_fs_branch::create()
{
  return std::shared_ptr<mem_fs_branch>(new mem_fs_branch());
}

mem_fs_branch::~mem_fs_branch()
{

}

/// @brief Create a child of this mem_fs_branch
///
/// A new in-memory file is created.
///
/// @param[out] leaf Storage for the newly created leaf.
///
/// @return A suitable error code.
ERR_CODE mem_fs_branch::create_child_here(std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  ERR_CODE result = ERR_CODE::NO_ERROR;

  KL_TRC_ENTRY;

  leaf = std::make_shared<mem_fs_leaf>(shared_from_this());

  if (leaf == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to create leaf object\n");
    result = ERR_CODE::OUT_OF_RESOURCE;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Standard constructor
///
/// @param parent Pointer to the parent branch.
mem_fs_leaf::mem_fs_leaf(std::shared_ptr<mem_fs_branch> parent) :
  _parent(std::weak_ptr<mem_fs_branch>(parent)),
  _buffer(nullptr),
  _buffer_length(0)
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_init(_lock);

  KL_TRC_EXIT;
}

mem_fs_leaf::~mem_fs_leaf()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
}

ERR_CODE mem_fs_leaf::read_bytes(uint64_t start,
                                 uint64_t length,
                                 uint8_t *buffer,
                                 uint64_t buffer_length,
                                 uint64_t &bytes_read)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::INVALID_OP;

  if (buffer == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "buffer is nullptr\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    result = ERR_CODE::NO_ERROR;

    klib_synch_spinlock_lock(this->_lock);

    if (start > _buffer_length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Start address outside range\n");
      bytes_read = 0;
    }
    else
    {
      if ((start + length) > _buffer_length)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Truncating read\n");
        length = _buffer_length - start;
      }

      if (length > buffer_length)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Filling buffer\n");
        length = buffer_length;
      }

      kl_memcpy(_buffer.get() + start, buffer, length);
      bytes_read = length;
    }

    klib_synch_spinlock_unlock(this->_lock);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE mem_fs_leaf::write_bytes(uint64_t start,
                                  uint64_t length,
                                  const uint8_t *buffer,
                                  uint64_t buffer_length,
                                  uint64_t &bytes_written)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::NO_ERROR;

  klib_synch_spinlock_lock(this->_lock);

  if (buffer_length < length)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Resetting length to buffer length\n");
    length = buffer_length;
  }

  if (start + length > this->_buffer_length)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Increasing buffer size\n");
    this->_no_lock_set_file_size(start + length);
  }

  ASSERT(start + length <= _buffer_length);
  kl_memcpy(buffer, _buffer.get() + start, length);

  bytes_written = length;

  klib_synch_spinlock_unlock(this->_lock);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ERR_CODE mem_fs_leaf::get_file_size(uint64_t &file_size)
{
  KL_TRC_ENTRY;

  file_size = this->_buffer_length;

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}

ERR_CODE mem_fs_leaf::set_file_size(uint64_t file_size)
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(this->_lock);
  this->_no_lock_set_file_size(file_size);
  klib_synch_spinlock_unlock(this->_lock);

  KL_TRC_EXIT;
  return ERR_CODE::NO_ERROR;
}

/// @brief Carry out that actual actions for resetting the memory file's size.
///
/// This function has no locking - it is meant to be called from the public functions of this class which do enforce
/// appropriate locking.
///
/// @param file_size The size to set the file to.
void mem_fs_leaf::_no_lock_set_file_size(uint64_t file_size)
{
  KL_TRC_ENTRY;

  uint64_t copy_size;

  char *new_buffer = new char [file_size];

  if (file_size < _buffer_length)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "New file size is smaller\n");
    copy_size = file_size;
  }
  else
  {
    copy_size = _buffer_length;
  }

  kl_memcpy(_buffer.get(), new_buffer, copy_size);

  if (file_size > _buffer_length)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Extending file\n");
    kl_memset(new_buffer + _buffer_length, 0, file_size - _buffer_length);
  }

  _buffer_length = file_size;
  _buffer.reset(new_buffer);

  KL_TRC_EXIT;
}