/// @file
/// @brief Basic tests of the work queue system.

#include "gtest/gtest.h"
#include "test/test_core/test.h"
#include "processor/processor.h"
#include "processor/work_queue.h"

#include <thread>
#include <iostream>

using namespace std;

/// @brief Unit test response class.
///
class WorkQueueTestResponseClass : public work::work_response
{
public:
  uint64_t response_value;
};

/// @brief
class WorkQueueTestWorkItemClass : public work::work_item
{
public:
  WorkQueueTestWorkItemClass()
  {
    typed_response = std::make_shared<WorkQueueTestResponseClass>();
    response_item = std::dynamic_pointer_cast<work::work_response>(typed_response);

    if (response_item == nullptr)
    {
      throw std::runtime_error("Wrong response type");
    }
  }

  uint64_t request_value;
  std::shared_ptr<WorkQueueTestResponseClass> typed_response;
};

/// @brief Simple work handler class for the unit tests.
///
/// The "handling" that occurs here is simply to copy the work item request value to the response.
class WorkQueueTestHandlerClass : public work::worker_object
{
public:
  virtual void handle_work_item(std::shared_ptr<work::work_item> item) override
  {
    std::shared_ptr<WorkQueueTestWorkItemClass> real_item =std::dynamic_pointer_cast<WorkQueueTestWorkItemClass>(item);
    if (real_item == nullptr)
    {
      throw std::runtime_error("Wrong work item type");
    }

    real_item->typed_response->response_value = real_item->request_value;
  };
};

/// @brief The class containing the actual tests.
///
class WorkQueueTests : public ::testing::Test
{
  void SetUp() override
  {
    work_queue_thread = new std::thread(work::work_queue_thread);
  }

  void TearDown() override
  {
    test_only_reset_work_queue();

    work_queue_thread->join();
    delete work_queue_thread;
    work_queue_thread = nullptr;
  }

  std::thread *work_queue_thread;
};

TEST_F(WorkQueueTests, SingleQueuedItem)
{
  std::shared_ptr<WorkQueueTestHandlerClass> handler = std::make_shared<WorkQueueTestHandlerClass>();
  std::shared_ptr<WorkQueueTestWorkItemClass> item = std::make_shared<WorkQueueTestWorkItemClass>();

  item->request_value = 5;
  work::queue_work_item(handler, item);
  item->response_item->wait_for_response();
}
