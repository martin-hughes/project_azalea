#include "test/test_core/test.h"

#include "system_tree/system_tree.h"
#include "system_tree/fs/mem/mem_fs.h"
#include "user_interfaces/syscall.h"

#include "gtest/gtest.h"

using namespace std;

// Tests of the Mem FS filesystem via the system call interface. This allows checking of object lifetimes, etc.

class MemFsSyscallTests : public ::testing::Test
{
protected:
  shared_ptr<task_process> sys_proc;
  shared_ptr<mem_fs_branch> root_branch;

  void SetUp() override
  {
    ERR_CODE ec;

    hm_gen_init();
    system_tree_init();

    sys_proc = task_init();

    test_only_set_cur_thread(sys_proc->child_threads.head->item.get());

    root_branch = mem_fs_branch::create();

    ec = system_tree()->add_child("mem", root_branch);
    ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  };

  void TearDown() override
  {
    test_only_set_cur_thread(nullptr);

    system_tree()->delete_child("mem");

    root_branch = nullptr;
    sys_proc = nullptr;

    test_only_reset_task_mgr();
    test_only_reset_system_tree();
  };
};

TEST_F(MemFsSyscallTests, CreateAndExit)
{
  ERR_CODE ec;
  char filename[] = "mem\\new_file";
  GEN_HANDLE new_file_handle;

  ec = syscall_create_obj_and_handle(filename, strlen(filename), &new_file_handle);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
}

TEST_F(MemFsSyscallTests, CreateWriteAndRead)
{
  ERR_CODE ec;
  char filename[] = "mem\\new_file";
  unsigned char test_string[23] = "This is a test string.";
  unsigned char output_buffer[23];
  GEN_HANDLE new_file_handle;
  uint64_t br;

  memset(output_buffer, 0, 23);

  ec = syscall_create_obj_and_handle(filename, strlen(filename), &new_file_handle);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);

  ec = syscall_write_handle(new_file_handle, 0, 23, test_string, 23, &br);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(br, 23);

  br = 0;
  ec = syscall_get_handle_data_len(new_file_handle, &br);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(br, 23);

  ec = syscall_close_handle(new_file_handle);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  new_file_handle = 0;

  ec = syscall_open_handle(filename, strlen(filename), &new_file_handle);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);

  br = 0;
  ec = syscall_get_handle_data_len(new_file_handle, &br);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(br, 23);

  ec = syscall_read_handle(new_file_handle, 0, 23, output_buffer, 23, &br);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(br, 23);
  ASSERT_STREQ((char *)output_buffer, (char *)test_string);

  ec = syscall_close_handle(new_file_handle);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
}

TEST_F(MemFsSyscallTests, FileDoesntExist)
{
  char filename[] = "mem\\new_file";
  ERR_CODE ec;
  GEN_HANDLE fh = 123;

  ec = syscall_open_handle(filename, strlen(filename), &fh);
  ASSERT_EQ(ec, ERR_CODE::NOT_FOUND);
  ASSERT_EQ(fh, 123); 
}

/* There isn't the ability to delete files yet.
TEST_F(MemFsSyscallTests, CreateWriteDelete)
{
  char filename[] = "mem\\new_file";
  unsigned char test_string[23] = "This is a test string.";
  unsigned char output_buffer[23];
  GEN_HANDLE new_file_handle;
  uint64_t br;
  ERR_CODE ec;

  memset(output_buffer, 0, 23);

  ec = syscall_create_obj_and_handle(filename, strlen(filename), &new_file_handle);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);

  ec = syscall_write_handle(new_file_handle, 0, 23, test_string, 23, &br);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);
  ASSERT_EQ(br, 23);

  ec = syscall_close_handle(new_file_handle);
  ASSERT_EQ(ec, ERR_CODE::NO_ERROR);

  ...
} */