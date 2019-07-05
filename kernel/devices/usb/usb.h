/// @file
/// @brief Header file for all USB related functions.

#pragma once

#include <memory>

#include "processor/work_queue.h"
#include "devices/usb/usb_gen_device.h"

namespace usb
{
  /// @brief Creates objects to drive USB devices that get plugged in.
  ///
  /// This is a worker object so that devices can be initialised asynchronously once plugged in.
  class main_factory : public work::worker_object
  {
  public:
    main_factory() { };
    virtual ~main_factory() = default;

    static void create_device(std::shared_ptr<generic_core> device_core);

    virtual void handle_work_item(std::shared_ptr<work::work_item> item) override;

  protected:
    /// @brief A work item class allowing the initialisation of USB devices to continue asynchronously.
    ///
    class create_device_work_item : public work::work_item
    {
    public:
      create_device_work_item(std::shared_ptr<generic_core> core);

      std::shared_ptr<generic_core> device_core; ///< The core object of the device to instantiate.
    };

    void create_device_handler(std::shared_ptr<create_device_work_item> item);
  };

  void initialise_usb_system();
};