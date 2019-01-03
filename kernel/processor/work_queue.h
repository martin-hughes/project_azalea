/// @file
/// @brief Header describing the work queueing mechanism in Azalea.

#pragma once

#include <memory>
#include "processor/synch_objects.h"

namespace work
{
  /// @brief This object holds the output of any queued work.
  ///
  /// The caller can, if desired, wait on this object to be signalled, which will happen when the associated work item
  /// has been completed.
  class work_response : protected WaitForFirstTriggerObject
  {
  public:
    work_response();
    virtual ~work_response() = default;

    virtual void set_response_complete();
    virtual bool is_response_complete();
    virtual void wait_for_response();

  protected:
    /// Has this work item already been marked complete by having set_response_complete() being called? This is only
    /// really useful as a performance enhancement - if wait_for_response() is called twice, the second time will not
    /// block.
    bool response_marked_complete;
  };

  /// @brief Contains details of a single work item.
  ///
  /// This object is provided to the worker_object class that should handle it. It is expected that users of the work
  /// queue system will create objects that derive from work_item to represent the various tasks they may need to
  /// perform.
  class work_item
  {
  public:
    work_item();
    virtual ~work_item() = default;

    /// This item stores the response to this work item
    std::shared_ptr<work_response> response_item;
  };

  /// @brief The base class of any object wishing to use the work queue.
  ///
  /// Any object wishing to allow work using that object to be scheduled by the system work queue must inherit from
  /// this class.
  class worker_object
  {
  public:
    /// @brief Handle the provided work item.
    ///
    /// At the end of handling the item, if a response object is provided, the system work queue thread will signal the
    /// object to indicate to the calling thread that the work item has been handled.
    ///
    /// @param item The item that was put on the work queue for this object to handle.
    virtual void handle_work_item(std::shared_ptr<work_item> item) = 0;
  };

  void work_queue_thread();
  void queue_work_item(std::shared_ptr<worker_object> object, std::shared_ptr<work_item> item);
};

#ifdef AZALEA_TEST_CODE
void test_only_reset_work_queue();
#endif
