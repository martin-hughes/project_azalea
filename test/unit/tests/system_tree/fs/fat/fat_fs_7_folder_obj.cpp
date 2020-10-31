// Direct tests of the FAT folder object

#include "test_core/test.h"
#include "../system_tree/fs/fat/fat_fs.h"
#include "../system_tree/fs/fat/fat_internal.h"
#include "test/unit/dummy_libs/fat_fs/fake_fat_fs.h"

#include "processor.h"

using namespace std;

using system_class = test_system_factory<non_queueing, true, true>;

namespace
{
  fat::fat_dir_entry entries_list[] =
    {
      { false, "TEST    TXT" },
    };

  uint32_t const num_entries = sizeof(entries_list) / sizeof(fat::fat_dir_entry);
}

class FatFsFolderTests : public ::testing::Test
{
protected:
  shared_ptr<system_class> test_system;

  shared_ptr<fat::fake_fat_fs> underlying_fs;
  shared_ptr<fat::folder> test_folder;
  shared_ptr<fat::pseudo_folder> folder_file;

  void SetUp() override
  {
    test_system = std::make_shared<system_class>();

    underlying_fs = fat::fake_fat_fs::create();
    folder_file = fat::pseudo_folder::create(entries_list, num_entries);

    test_folder = fat::folder::create(folder_file, underlying_fs);
  }

  void TearDown() override
  {
    test_system = nullptr;
  }
};

TEST_F(FatFsFolderTests, BasicLookup)
{
  ERR_CODE result;

  std::shared_ptr<IHandledObject> obj;
  result = test_folder->get_child("TEST.TXT", obj);

  ASSERT_EQ(result, ERR_CODE::NO_ERROR);
}
