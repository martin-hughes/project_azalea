#include "test_core/test.h"
#include "system_tree.h"
#include "../system_tree/fs/pipe/pipe_fs.h"

#include "gtest/gtest.h"

using namespace std;

const uint64_t pipe_size = 1 << 10;
const uint64_t buffer_size = 50;

// A simple test of the pipes objects within ST.
TEST(SystemTreeTest, GeneralPipes)
{
  std::shared_ptr<pipe_branch> pipe_obj = pipe_branch::create();
  shared_ptr<pipe_branch::pipe_read_leaf> reader;
  shared_ptr<pipe_branch::pipe_write_leaf> writer;
  shared_ptr<IHandledObject> leaf;
  std::unique_ptr<uint8_t[]> buf(new uint8_t [buffer_size]);
  uint64_t total_written = 0;
  uint64_t written_this_time = 0;
  uint64_t total_read = 0;
  uint64_t read_this_time = 0;
  ERR_CODE r;

  for (int i = 0; i < buffer_size; i++)
  {
    buf[i] = i;
  }

  // Start with some simple checks on what leaves are available
  r = pipe_obj->get_child("nope", leaf);
  ASSERT_EQ(r, ERR_CODE::NOT_FOUND);

  r = pipe_obj->get_child("read", leaf);
  ASSERT_EQ(r, ERR_CODE::NO_ERROR);
  ASSERT_TRUE(leaf);
  reader = dynamic_pointer_cast<pipe_branch::pipe_read_leaf>(leaf);
  ASSERT_TRUE(reader);

  r = pipe_obj->get_child("write", leaf);
  ASSERT_EQ(r, ERR_CODE::NO_ERROR);
  ASSERT_TRUE(leaf);
  writer = dynamic_pointer_cast<pipe_branch::pipe_write_leaf>(leaf);
  ASSERT_TRUE(reader);

  for (int i = 0; i < pipe_size; i += buffer_size)
  {
    r = writer->write_bytes(0, buffer_size, buf.get(), buffer_size, written_this_time);
    ASSERT_EQ(r, ERR_CODE::NO_ERROR);
    total_written += written_this_time;
  }
  ASSERT_EQ(total_written, pipe_size);

  r = writer->write_bytes(0, buffer_size, buf.get(), buffer_size, written_this_time);
  ASSERT_EQ(r, ERR_CODE::NO_ERROR);
  ASSERT_EQ(written_this_time, 0);

  for (int i = 0; i < pipe_size; i += buffer_size)
  {
    r = reader->read_bytes(0, buffer_size, buf.get(), buffer_size, read_this_time);
    ASSERT_EQ(r, ERR_CODE::NO_ERROR);
    total_read += read_this_time;
  }
  ASSERT_EQ(total_read, pipe_size);

  r = reader->read_bytes(0, buffer_size, buf.get(), buffer_size, read_this_time);
  ASSERT_EQ(r, ERR_CODE::NO_ERROR);
  ASSERT_EQ(read_this_time, 0);
}
