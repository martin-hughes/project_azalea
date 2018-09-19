/// @brief Implementation of pipes for use in IPC.

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

pipe_branch::pipe_branch() : _buffer(new uint8_t[NORMAL_BUFFER_SIZE])
{
  KL_TRC_ENTRY;

  _read_ptr = _buffer.get();
  _write_ptr = _read_ptr;

  klib_synch_spinlock_init(this->_pipe_lock);

  KL_TRC_EXIT;
}


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

ERR_CODE pipe_branch::get_child_type(const kl_string &name, CHILD_TYPE &type)
{
  ERR_CODE ret = ERR_CODE::NOT_FOUND;

  KL_TRC_ENTRY;

  type = CHILD_TYPE::NOT_FOUND;
  if ((name == read_leaf_name) || (name == write_leaf_name))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Matches read or write leaf names\n");
    type = CHILD_TYPE::LEAF;
    ret = ERR_CODE::NO_ERROR;
  }

  KL_TRC_EXIT;

  return ret;
}

ERR_CODE pipe_branch::get_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  // Always return NOT_FOUND because there are no child branches to any pipe.
  return ERR_CODE::NOT_FOUND;
}

ERR_CODE pipe_branch::get_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  ERR_CODE ret = ERR_CODE::NOT_FOUND;
  KL_TRC_ENTRY;

  if (name == read_leaf_name)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Requested read leaf\n");
    leaf = std::make_shared<pipe_branch::pipe_read_leaf>(shared_from_this());
    ret = ERR_CODE::NO_ERROR;
  }
  else if (name == write_leaf_name)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Requested write leaf\n");
    leaf = std::make_shared<pipe_branch::pipe_write_leaf>(shared_from_this());
    ret = ERR_CODE::NO_ERROR;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", ret, "\n");
  KL_TRC_EXIT;

  return ret;
}

ERR_CODE pipe_branch::add_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> branch)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  // Pipes cannot contain child branches.
  return ERR_CODE::INVALID_OP;
}

ERR_CODE pipe_branch::add_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> leaf)
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

pipe_branch::pipe_read_leaf::pipe_read_leaf(std::shared_ptr<pipe_branch> parent) : 
  _parent(std::weak_ptr<pipe_branch>(parent))
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
    KL_TRC_TRACE(TRC_LVL::FLOW, "Try to read from the pipe\n");
    klib_synch_spinlock_lock(parent_branch->_pipe_lock);

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

    read_length = avail_length;
    if (buffer_length < read_length)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Truncate read to buffer length\n");
      read_length = buffer_length;
    }
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

    klib_synch_spinlock_unlock(parent_branch->_pipe_lock);

    ret = ERR_CODE::NO_ERROR;
  }

  KL_TRC_EXIT;

  return ret;
}

ERR_CODE pipe_branch::create_branch(const kl_string &name, std::shared_ptr<ISystemTreeBranch> &branch)
{
  // You can't add extra branches to a pipe branch.
  return ERR_CODE::INVALID_OP;
}

ERR_CODE pipe_branch::create_leaf(const kl_string &name, std::shared_ptr<ISystemTreeLeaf> &leaf)
{
  // You can't add extra branches to a pipe branch.
  return ERR_CODE::INVALID_OP;
}
