#include "test/test_core/test.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/mem/mem_fs.h"

#include "gtest/gtest.h"

using namespace std;

// A simple test of the mem FS objects within ST.
TEST(MemFsBasicTests, CreateAndDestroy)
{
  mem_fs_leaf test_leaf(nullptr);
}

TEST(MemFsBasicTests, EmptyRead)
{
  mem_fs_leaf test_leaf(nullptr);
  uint64_t bytes_read = 0;
  uint8_t buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} ;
  ERR_CODE ec;

  ec = test_leaf.read_bytes(0, 5, buffer, 10, bytes_read);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_read, 0);
  ASSERT_EQ(buffer[2], 3);
}

TEST(MemFsBasicTests, SimpleWriteAndRead)
{
  mem_fs_leaf test_leaf(nullptr);

  uint64_t bytes_done = 0;
  uint8_t buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  uint8_t expected_out[10] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 5};
  ERR_CODE ec;

  ec = test_leaf.write_bytes(5, 5, buffer, 10, bytes_done);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_done, 5);

  ec = test_leaf.read_bytes(0, 10, buffer, 10, bytes_done);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_done, 10);
  for (int i = 0; i < 10; i++)
  {
    ASSERT_EQ(buffer[i], expected_out[i]);
  }
}

TEST(MemFsBasicTests, WriteAndLongRead)
{
  mem_fs_leaf test_leaf(nullptr);

  uint64_t bytes_done = 0;
  uint8_t buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  uint8_t expected_out[10] = {0, 0, 1, 2, 3, 0, 0, 0, 0, 0};
  ERR_CODE ec;

  ec = test_leaf.write_bytes(2, 3, buffer, 10, bytes_done);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_done, 3);

  ec = test_leaf.read_bytes(0, 10, buffer, 10, bytes_done);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_done, 5);
  for (int i = 0; i < 5; i++)
  {
    ASSERT_EQ(buffer[i], expected_out[i]);
  }
}

TEST(MemFsBasicTests, SetFileSize)
{
  mem_fs_leaf test_leaf(nullptr);

  uint64_t bytes_done = 0;
  uint8_t buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  uint8_t expected_out[10] = {1, 2, 3, 4, 5, 0, 0, 0, 0, 0};
  ERR_CODE ec;

  ec = test_leaf.write_bytes(0, 10, buffer, 10, bytes_done);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_done, 10);

  test_leaf.set_file_size(5);
  ec = test_leaf.read_bytes(0, 10, buffer, 10, bytes_done);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_done, 5);

  test_leaf.set_file_size(10);
  ec = test_leaf.read_bytes(0, 10, buffer, 10, bytes_done);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(bytes_done, 10);

  for (int i = 0; i < 10; i++)
  {
    ASSERT_EQ(buffer[i], expected_out[i]);
  }
}