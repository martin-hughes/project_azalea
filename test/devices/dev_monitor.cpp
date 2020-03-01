#include "test/test_core/test.h"
#include "gtest/gtest.h"

#include "devices/device_monitor.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/dev/dev_fs.h"

#include <memory>

extern std::shared_ptr<terms::generic> *term_ptr;
std::shared_ptr<terms::generic> *term_ptr{nullptr};

class DeviceMonitorTest : public ::testing::Test
{
protected:
  virtual void SetUp() override;
  virtual void TearDown() override;

  std::shared_ptr<system_tree_simple_branch> dev_root;
};

class DummyDevice : public IDevice
{
public:
  DummyDevice() : IDevice{"Dummy Device", "dd", true} { };
  virtual ~DummyDevice() override = default;

  virtual bool start() override { return true; };
  virtual bool stop() override { return true; };
  virtual bool reset() override { return true; };
};

void DeviceMonitorTest::SetUp()
{
  system_tree_init();
  dev_root = std::make_shared<system_tree_simple_branch>();
  ASSERT_EQ(system_tree()->add_child("\\dev", dev_root), ERR_CODE::NO_ERROR);
  work::init_queue();
  dev::monitor::init();
}

void DeviceMonitorTest::TearDown()
{
  dev::monitor::terminate();
  work::test_only_terminate_queue();
  test_only_reset_system_tree();
  test_only_reset_name_counts();
}

TEST_F(DeviceMonitorTest, SimpleRegister)
{
  std::shared_ptr<IDevice> empty;
  std::shared_ptr<DummyDevice> dd;
  ASSERT(dev::create_new_device(dd, empty));
}
