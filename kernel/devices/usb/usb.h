/// @file
/// @brief Header file for all USB related functions.

#pragma once

#include <memory>

#include "work_queue.h"
#include "usb_gen_device.h"

namespace usb
{
  /// @brief Creates objects to drive USB devices that get plugged in.
  ///
  /// This is a worker object so that devices can be initialised asynchronously once plugged in.
  class main_factory : public work::message_receiver
  {
  public:
    /// @brief Enumeration of the necessary steps to bringing up a USB device.
    enum class CREATION_PHASE
    {
      NOT_STARTED, ///< No steps have been completed
      DISCOVERY_COMPLETE, ///< Discovery is completed
      DEVICE_CONFIGURED, ///< The device configuration has been selected and sent to the USB device
      COMPLETE, ///< Device setup is complete
    };

    main_factory() { };
    virtual ~main_factory() = default;

    static void create_device(std::shared_ptr<generic_core> device_core,
                              CREATION_PHASE phase = CREATION_PHASE::NOT_STARTED);

    virtual void handle_message(std::unique_ptr<msg::root_msg> message) override;

  protected:

    /// @brief A work item class allowing the initialisation of USB devices to continue asynchronously.
    ///
    class create_device_work_item : public msg::root_msg
    {
    public:
      create_device_work_item(std::shared_ptr<generic_core> core, CREATION_PHASE phase);

      std::shared_ptr<generic_core> device_core; ///< The core object of the device to instantiate.
      CREATION_PHASE cur_phase; ///< Current phase of constructing a USB device core.
    };

    void create_device_handler(std::unique_ptr<create_device_work_item> &item);
  };

  void initialise_usb_system();
};