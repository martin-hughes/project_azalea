/// @file
/// @brief Implementation of pipes for use in IPC.

// Known defects:
// - block_on_read is untested
// - message send on write is untested.
// - Does the parent weak_ptr need lock protection in case it is overwritten and locked at the same time?

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "system_tree/fs/pipe/pipe_fs.h"

using namespace std;

namespace
{
  const uint64_t NORMAL_BUFFER_SIZE = 1 << 10;
  const char read_leaf_name[] = "read";
  const char write_leaf_name[] = "write";
}

/// @brief Standard constructor
///
/// New pipe branches should be created using the static create() function.
pipe_branch::pipe_branch() : _buffer(new uint8_t[NORMAL_BUFFER_SIZE])
{
  KL_TRC_ENTRY;

  _read_ptr = _buffer.get();
  _write_ptr = _read_ptr;

  klib_synch_spinlock_init(this->_pipe_lock);

  KL_TRC_EXIT;
}

/// @brief Create a new pipe branch.
///
/// @return shared_ptr of the new branch.
std::shared_ptr<pipe_branch> pipe_branch::create()
{
  return std::shared_ptr<pipe_branch>(new pipe_branch());
}

// This fails for the time being because we don't yet have a way of removing objects from object_mgr, so child objects
// can still be in use after being deleted.
pipe_branch::~pipe_branch()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::ERROR, "Improper cleanup of pipes!\n");
  KL_TRC_EXIT;
}

ERR_CODE pipe_branch::get_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  ERR_CODE ret = ERR_CODE::NOT_FOUND;
  KL_TRC_ENTRY;

  if (name == read_leaf_name)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Requested read leaf\n");
    child = std::make_shared<pipe_branch::pipe_read_leaf>(shared_from_this());
    ret = ERR_CODE::NO_ERROR;
  }
  else if (name == write_leaf_name)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Requested write leaf\n");
    child = std::make_shared<pipe_branch::pipe_write_leaf>(shared_from_this());
    ret = ERR_CODE::NO_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ret, "\n");
  KL_TRC_EXIT;

  return ret;
}

ERR_CODE pipe_branch::add_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> child)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  // There is no need to ever be able to add an extra leaf to a pipe.
  return ERR_CODE::INVALID_OP;
}

ERR_CODE pipe_branch::rename_child(const kl_string &old_name, const kl_string &new_name)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  // The leaves of a pipe have a constant name, so don't permit renaming them.
  return ERR_CODE::INVALID_OP;
}

ERR_CODE pipe_branch::delete_child(const kl_string &name)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  // The leaves of a pipe are both required and cannot be deleted without deleting the whole pipe.
  return ERR_CODE::INVALID_OP;
}

/// @brief Set the object that should receive a message when new data is added to this pipe.
///
/// @param new_handler The object to receive the message
void pipe_branch::set_msg_receiver(std::shared_ptr<work::message_receiver> &new_handler)
{
  KL_TRC_ENTRY;

  new_data_handler = new_handler;

  KL_TRC_EXIT;
}

/// @brief Standard constructor
///
/// @param parent The parent pipe_branch.
pipe_branch::pipe_read_leaf::pipe_read_leaf(std::shared_ptr<pipe_branch> parent) :
  _parent(std::weak_ptr<pipe_branch>(parent)), block_on_read(false)
{
  ASSERT(parent != nullptr);
}
pipe_branch::pipe_read_leaf::~pipe_read_leaf()
{

}

ERR_CODE pipe_branch::pipe_read_leaf::read_bytes(uint64_t start,
                                                 uint64_t length,
                                                 uint8_t *buffer,
                                                 uint64_t buffer_length,
                                                 uint64_t &bytes_read)
{
  ERR_CODE ret = ERR_CODE::UNKNOWN;
  uint64_t read_length;
  uint64_t avail_length;
  uint64_t rp;
  uint64_t wp;
  uint64_t buf_start;

  std::shared_ptr<pipe_branch> parent_branch = this->_parent.lock();

  KL_TRC_ENTRY;

  if (!parent_branch)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent branch deleted\n");
    ret = ERR_CODE::INVALID_OP;
  }
  else if (buffer == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid buffer ptr\n");
    ret = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Try to read ", length, " bytes from the pipe\n");

    if (length > buffer_length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Truncate to buffer_length\n");
      length = buffer_length;
    }

    klib_synch_spinlock_lock(parent_branch->_pipe_lock);

    while (1)
    {
      rp = reinterpret_cast<uint64_t>(parent_branch->_read_ptr);
      wp = reinterpret_cast<uint64_t>(parent_branch->_write_ptr);
      buf_start = reinterpret_cast<uint64_t>(parent_branch->_buffer.get());

      if (wp >= rp)
      {
        avail_length = wp - rp;
      }
      else
      {
        avail_length = (NORMAL_BUFFER_SIZE + wp) - rp;
      }
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Available bytes to read: ", avail_length, "\n");

      if (this->block_on_read && (avail_length < length))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Waiting for more bytes\n");
        klib_synch_spinlock_unlock(parent_branch->_pipe_lock);
        task_yield();
        klib_synch_spinlock_lock(parent_branch->_pipe_lock);
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Got sufficient bytes now\n");
        break;
      }
    }

    read_length = avail_length;
    if (length < read_length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Truncate read to requested length\n");
      read_length = length;
    }

    // This isn't particularly elegant, but it'll do for now.
    // Perhaps we could make a generic ring-buffer?
    bytes_read = 0;
    for (int i = 0; i < read_length; i++, bytes_read++)
    {
      *buffer = *parent_branch->_read_ptr;
      buffer++;
      parent_branch->_read_ptr++;
      rp = reinterpret_cast<uint64_t>(parent_branch->_read_ptr);
      if (rp > buf_start + NORMAL_BUFFER_SIZE)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Read pointer wrapped\n");
        parent_branch->_read_ptr = parent_branch->_buffer.get();
      }
    }

    ASSERT(read_length == bytes_read);

    klib_synch_spinlock_unlock(parent_branch->_pipe_lock);

    ret = ERR_CODE::NO_ERROR;
  }

  KL_TRC_EXIT;

  return ret;
}

/// @brief Set whether or not to block reads to wait for data.
///
/// If the pipe is set to blocking, then reads will not return until they can return the number of bytes requested.
///
/// @param block Should this pipe block reads?
void pipe_branch::pipe_read_leaf::set_block_on_read(bool block)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "New blocking state: ", block, "\n");
  this->block_on_read = block;
  KL_TRC_EXIT;
}

/// @brief Standard constructor
///
/// @param parent The parent pipe_branch.
pipe_branch::pipe_write_leaf::pipe_write_leaf(std::shared_ptr<pipe_branch> parent) :
  _parent(std::weak_ptr<pipe_branch>(parent))
{
  ASSERT(parent != nullptr);
}

pipe_branch::pipe_write_leaf::~pipe_write_leaf()
{

}

ERR_CODE pipe_branch::pipe_write_leaf::write_bytes(uint64_t start,
                                                   uint64_t length,
                                                   const uint8_t *buffer,
                                                   uint64_t buffer_length,
                                                   uint64_t &bytes_written)
{
  ERR_CODE ret = ERR_CODE::UNKNOWN;
  uint64_t write_length;
  uint64_t avail_length;
  uint64_t rp;
  uint64_t wp;
  uint64_t buf_start;
  std::shared_ptr<pipe_branch> parent_branch = this->_parent.lock();

  KL_TRC_ENTRY;

  if (!parent_branch)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent branch deleted\n");
    ret = ERR_CODE::INVALID_OP;
  }
  else if (buffer == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid buffer ptr\n");
    ret = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Try to write from the pipe\n");
    klib_synch_spinlock_lock(parent_branch->_pipe_lock);

    rp = reinterpret_cast<uint64_t>(parent_branch->_read_ptr);
    wp = reinterpret_cast<uint64_t>(parent_branch->_write_ptr);
    buf_start = reinterpret_cast<uint64_t>(parent_branch->_buffer.get());

    if (rp > wp)
    {
      avail_length = rp - wp;
    }
    else
    {
      avail_length = (NORMAL_BUFFER_SIZE + rp) - wp;
    }
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Available bytes to write: ", avail_length, "\n");

    write_length = avail_length;
    if (buffer_length < write_length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Truncate write to buffer length\n");
      write_length = buffer_length;
    }
    if (length < write_length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Truncate read to requested length\n");
      write_length = length;
    }

    // This isn't particularly elegant, but it'll do for now.
    // Perhaps we could make a generic ring-buffer?
    bytes_written = 0;
    for (int i = 0; i < write_length; i++, bytes_written++)
    {
      *parent_branch->_write_ptr = *buffer;
      buffer++;
      parent_branch->_write_ptr++;
      wp = reinterpret_cast<uint64_t>(parent_branch->_write_ptr);
      if (wp > buf_start + NORMAL_BUFFER_SIZE)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Write pointer wrapped\n");
        parent_branch->_write_ptr = parent_branch->_buffer.get();
      }
    }

    ASSERT(write_length == bytes_written);

    std::shared_ptr<work::message_receiver> rcv = parent_branch->new_data_handler.lock();
    if (rcv)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Send new data message\n");
      std::unique_ptr<msg::root_msg> msg = std::make_unique<msg::root_msg>();
      msg->message_id = SM_PIPE_NEW_DATA;
      work::queue_message(rcv, std::move(msg));
    }

    klib_synch_spinlock_unlock(parent_branch->_pipe_lock);

    ret = ERR_CODE::NO_ERROR;
  }

  KL_TRC_EXIT;

  return ret;
}

ERR_CODE pipe_branch::create_child(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &child)
{
  // You can't add extra children to a pipe branch.
  return ERR_CODE::INVALID_OP;
}
