#include "test/system_tree/system_tree_tests.h"
#include "test/test_core/test.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/pipe/pipe_fs.h"

#include "gtest/gtest.h"
#include <iostream>

using namespace std;

const unsigned long pipe_size = 1 << 10;
const unsigned long buffer_size = 50;

// A simple test of the pipes objects within ST.
void system_tree_test_3_pipes()
{
  pipe_branch pipe_obj;
  pipe_branch::pipe_read_leaf *reader;
  pipe_branch::pipe_write_leaf *writer;
  ISystemTreeLeaf *leaf;
  std::unique_ptr<unsigned char[]> buf(new unsigned char [buffer_size]);
  unsigned long total_written = 0;
  unsigned long written_this_time = 0;
  unsigned long total_read = 0;
  unsigned long read_this_time = 0;
  ERR_CODE r;

  for (int i = 0; i < buffer_size; i++)
  {
    buf[i] = i;
  }

  // Start with some simple checks on what leaves are available
  r = pipe_obj.get_leaf("nope", &leaf);
  ASSERT_EQ(r, ERR_CODE::NOT_FOUND);

  r = pipe_obj.get_leaf("read", &leaf);
  ASSERT_EQ(r, ERR_CODE::NO_ERROR);
  ASSERT_NE(leaf, nullptr);
  reader = dynamic_cast<pipe_branch::pipe_read_leaf *>(leaf);
  ASSERT_NE(reader, nullptr);

  r = pipe_obj.get_leaf("write", &leaf);
  ASSERT_EQ(r, ERR_CODE::NO_ERROR);
  ASSERT_NE(leaf, nullptr);
  writer = dynamic_cast<pipe_branch::pipe_write_leaf *>(leaf);
  ASSERT_NE(reader, nullptr);

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

  delete reader;
  delete writer;
}
