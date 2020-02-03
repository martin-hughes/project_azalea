/// @file
/// @brief Implements an xHCI-specific USB device core.

// Known deficiencies
// - device_request doesn't really calculate the correct TRT value.
// - Transfer failures result in an assert.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "devices/usb/usb_xhci_device.h"

using namespace usb::xhci;

/// @brief Standard Constructor
///
/// @param parent The parent controller, must not be nullptr.
///
/// @param port The ID of the port on the controller this device is connected to.
///
/// @param parent_port Pointer to the port object in the controller that this device is connected to.
device_core::device_core(controller *parent, uint8_t port, root_port *parent_port) :
  parent{parent},
  port_num{port},
  parent_port{parent_port},
  last_known_state{DEV_STATE::UNKNOWN},
  slot_id{0},
  current_max_packet_size{0},
  dev_context{nullptr}
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_init(current_transfers_lock);

  last_known_state = DEV_STATE::CREATE_CONTEXT;
  parent->request_slot(this);

  KL_TRC_EXIT;
}

device_core::~device_core()
{
  KL_TRC_ENTRY;

  // The command queue refers to this object by raw pointer, so if there were any commands queued that refer to this
  // device then they now have dangling pointers - not good.
  INCOMPLETE_CODE("Still need to ensure all commands retired.");

  KL_TRC_EXIT;
}

bool device_core::device_request(device_request_type request_type,
                                 uint8_t request,
                                 uint16_t value,
                                 uint16_t index,
                                 uint16_t length,
                                 void *buffer)
{
  bool result = true;
  bool requires_data_trb = false;
  uint64_t buffer_phys_addr;
  setup_stage_transfer_trb setup_trb;
  data_stage_transfer_trb data_trb;
  status_stage_transfer_trb status_trb;
  uint8_t trt_value = 0;
  uint16_t num_packets;
  uint64_t status_stage_phys_addr;
  std::shared_ptr<normal_transfer> transfer_item;

  KL_TRC_ENTRY;

  if ((request == DEV_REQUEST::GET_CONFIGURATION) ||
      (request == DEV_REQUEST::GET_DESCRIPTOR) ||
      (request == DEV_REQUEST::GET_INTERFACE) ||
      (request == DEV_REQUEST::GET_STATUS) ||
      (request == DEV_REQUEST::SET_DESCRIPTOR) ||
      (request == DEV_REQUEST::SET_SEL) ||
      (request == DEV_REQUEST::SYNCH_FRAME))
  {
    if ((buffer == nullptr) || (length == 0))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Requires buffer and data\n");
      result = false;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Add data frame\n");
      requires_data_trb = true;
      trt_value = 3;
    }
  }

  if (result == true)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parameter checks passed\n");

    setup_trb.populate(request_type.raw, request, value, index, length, 0, false, false, true, trt_value);

    if (requires_data_trb)
    {
      num_packets = length / current_max_packet_size;
      if ((length % current_max_packet_size) != 0)
      {
        num_packets++;
      }

      KL_TRC_TRACE(TRC_LVL::FLOW, "Scheduling a data TRB with ", num_packets, " packets\n");

      buffer_phys_addr = reinterpret_cast<uint64_t>(mem_get_phys_addr(buffer));
      data_trb.populate(buffer_phys_addr,
                        length,
                        num_packets,
                        0,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        true);
    }

    status_trb.populate(0, false, false, false, true, true);

    klib_synch_spinlock_lock(current_transfers_lock);
    result = def_ctrl_endpoint_transfer_ring->queue_ctrl_transfer(&setup_trb,
                                                                  (requires_data_trb ? &data_trb : nullptr),
                                                                  &status_trb,
                                                                  (requires_data_trb ? 1 : 0),
                                                                  status_stage_phys_addr);

    if (result)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer queued, add to responses list\n");
      transfer_item = std::make_shared<normal_transfer>(nullptr, nullptr, 0);
      ASSERT(map_contains(current_transfers, status_stage_phys_addr));
      current_transfers.insert({status_stage_phys_addr, transfer_item});
    }

    klib_synch_spinlock_unlock(current_transfers_lock);

    // Ring doorbell of transfer ring.
    parent->ring_doorbell(this->slot_id, EP_DOORBELL_CODE::CONTROL_EP_0);

    // Wait for response.
    if (result)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Wait for response\n");
      transfer_item->wait_for_signal(WaitObject::MAX_WAIT);
      KL_TRC_TRACE(TRC_LVL::FLOW, "Got response\n");
    }
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Handles a slot enabled event associated with this device.
///
/// @param slot_id The slot ID of the generating slot.
///
/// @param new_output_context Pointer to the output context for this slot. The output context is controller by the
///                           controller.
void device_core::handle_slot_enabled(uint8_t slot_id, device_context *new_output_context)
{
  KL_TRC_ENTRY;

  if (last_known_state == DEV_STATE::CREATE_CONTEXT)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Slot enabled\n");

    dev_context = new_output_context;

    def_ctrl_endpoint_transfer_ring = std::make_unique<trb_transfer_ring>(128);

    last_known_state = DEV_STATE::ENABLED;
    current_max_packet_size = parent_port->get_default_max_packet_size();

    dev_input_context = std::make_unique<input_context>();
    memset(dev_input_context.get(), 0, sizeof(input_context));

    // Initialize the new input context as per the xHCI spec, section 4.3.3 ("Device Slot Initialization")
    dev_input_context->control.add_context_flags = 3;  // That is, set A0 and A1 to true.
    dev_input_context->device.slot.root_hub_port_number = this->port_num;
    dev_input_context->device.slot.route_string = 0;
    dev_input_context->device.slot.num_context_entries = 1;

    dev_input_context->device.ep_0_bi_dir.endpoint_type = EP_TYPES::CONTROL;
    dev_input_context->device.ep_0_bi_dir.max_packet_size = current_max_packet_size;
    dev_input_context->device.ep_0_bi_dir.max_burst_size = 0;
    dev_input_context->device.ep_0_bi_dir.tr_dequeue_phys_ptr =
      reinterpret_cast<uint64_t>(def_ctrl_endpoint_transfer_ring->get_phys_base_address());
    dev_input_context->device.ep_0_bi_dir.dequeue_cycle_state = 1;
    dev_input_context->device.ep_0_bi_dir.interval = 0;
    dev_input_context->device.ep_0_bi_dir.max_primary_streams = 0;
    dev_input_context->device.ep_0_bi_dir.mult = 0;
    dev_input_context->device.ep_0_bi_dir.error_count = 3;

    this->slot_id = slot_id;

    parent->address_device(this, reinterpret_cast<uint64_t>(mem_get_phys_addr(dev_input_context.get())), slot_id);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Spurious slot enabled event\n");
    last_known_state = DEV_STATE::UNKNOWN;
  }

  KL_TRC_EXIT;
}

/// @brief Handle the device becoming addressed
///
void device_core::handle_addressed()
{
  KL_TRC_ENTRY;

  if (last_known_state == DEV_STATE::ENABLED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Now addressed - new slot state: ", dev_context->slot.slot_state, "\n");
    last_known_state = DEV_STATE::ADDRESSED;

    parent_port->handle_child_device_addressed();
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Spurious device addressed event\n");
    last_known_state = DEV_STATE::UNKNOWN;
  }

  KL_TRC_EXIT;
}

/// @brief Returns the root port number of this device.
///
/// @return The root port number of this device.
uint8_t device_core::get_port_num()
{
  return this->port_num;
}

/// @brief Called to handle a Transfer Event generated by the parent controller.
///
/// @param trb The Transfer Event TRB that triggered this call.
void device_core::handle_transfer_event(transfer_event_trb &trb)
{
  std::shared_ptr<normal_transfer> response;

  KL_TRC_ENTRY;

  // We don't have any way of dealing with failure at the moment.
  ASSERT(trb.completion_code == C_CODES::SUCCESS);

  klib_synch_spinlock_lock(current_transfers_lock);
  if (map_contains(current_transfers, trb.trb_pointer))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Response item waiting\n");
    response = current_transfers.find(trb.trb_pointer)->second;
    current_transfers.erase(trb.trb_pointer);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No response item!\n");
  }

  klib_synch_spinlock_unlock(current_transfers_lock);

  if (response)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Signal completion to ", response.get(), "\n");
    response->set_response_complete();
  }

  KL_TRC_EXIT;
}

uint16_t device_core::get_max_packet_size()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return current_max_packet_size;
}

bool device_core::set_max_packet_size(uint16_t new_packet_size)
{
  bool result = true;

  KL_TRC_ENTRY;

  if (new_packet_size != current_max_packet_size)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Actually update max packet size!\n");
    dev_input_context->control.add_context_flags = 3;
    dev_input_context->device.ep_0_bi_dir.max_packet_size = new_packet_size;

    if (!this->parent->evaluate_context(this,
                                        reinterpret_cast<uint64_t>(mem_get_phys_addr(dev_input_context.get())),
                                        this->slot_id))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set max packet size failed.\n");
      result = false;
    }
    else
    {
      current_max_packet_size = new_packet_size;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool device_core::configure_device(uint8_t config_num)
{
  bool result = false;
  device_config *active_cfg_ptr = &configurations[config_num];
  endpoint_descriptor *endpoint = nullptr;
  endpoint_context *cur_endpoint_ctxt = nullptr;
  uint8_t addr;
  bool in_direction;
  uint32_t flag_number;

  KL_TRC_ENTRY;

  result = set_configuration(config_num);
  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device config set, configure endpoints\n");
    dev_input_context->control.add_context_flags = 1;

    for (uint8_t i = 0; i < active_cfg_ptr->desc.num_interfaces; i++)
    {
      for (uint8_t j = 0; j < active_cfg_ptr->interfaces[i].desc.num_endpoints; j++)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Configure endpoint ", i, ":", j);
        endpoint = &active_cfg_ptr->interfaces[i].endpoints[j];

        in_direction = ((endpoint->endpoint_address & 0x80) != 0); // In or out direction endpoint?
        addr = endpoint->endpoint_address & 0x0F; // Truncate reserved and ignored bits.

        transfer_rings[addr][in_direction ? 1 : 0] = std::make_unique<trb_transfer_ring>(1024);
        cur_endpoint_ctxt = in_direction ?
                                  &dev_input_context->device.endpoints[addr - 1].in :
                                  &dev_input_context->device.endpoints[addr - 1].out;

        flag_number = (addr * 2) + (in_direction ? 1 : 0);
        dev_input_context->control.add_context_flags |= (1 << flag_number);
        KL_TRC_TRACE(TRC_LVL::FLOW, ", new flags: ", dev_input_context->control.add_context_flags, "\n");

        switch (endpoint->attributes.transfer_type)
        {
        case 0: // Control endpoint.
          KL_TRC_TRACE(TRC_LVL::FLOW, "Control endpoint number: ", endpoint->endpoint_address, "\n");

          if (in_direction)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Tried to create an inwards-only control EP\n");
            result = false;
            break;
          }

          cur_endpoint_ctxt->endpoint_state = 0;
          cur_endpoint_ctxt->mult = 0;
          cur_endpoint_ctxt->max_primary_streams = 0;
          cur_endpoint_ctxt->linear_stream_array = 1;
          cur_endpoint_ctxt->interval = endpoint->service_interval;
          cur_endpoint_ctxt->max_esit_payload_hi = 0;
          cur_endpoint_ctxt->error_count = 0;
          cur_endpoint_ctxt->endpoint_type = EP_TYPES::CONTROL;
          cur_endpoint_ctxt->host_initiate_disable = 0;
          cur_endpoint_ctxt->max_burst_size = 0;
          cur_endpoint_ctxt->max_packet_size = endpoint->max_packet_size;
          cur_endpoint_ctxt->tr_dequeue_phys_ptr = reinterpret_cast<uint64_t>(
                                                   transfer_rings[addr][0]->get_phys_base_address());
          cur_endpoint_ctxt->dequeue_cycle_state = 1;
          // Ideally, we'd keep a track of this, but the spec says this is reasonable starting value:
          cur_endpoint_ctxt->average_trb_length = 8;
          cur_endpoint_ctxt->max_esit_payload_lo = 0;

          break;

        case 1: // Isochronous endpoint
        case 2: // Bulk endpoint
          INCOMPLETE_CODE("Unsupported endpoint type");
          break;

        case 3: // Interrupt endpoint
          KL_TRC_TRACE(TRC_LVL::FLOW, "Interrupt ", in_direction ? "IN" : "OUT", " endpoint number: ", addr, "\n");
          cur_endpoint_ctxt->endpoint_state = 0;
          cur_endpoint_ctxt->mult = 0;
          cur_endpoint_ctxt->max_primary_streams = 0;
          cur_endpoint_ctxt->linear_stream_array = 1;
          cur_endpoint_ctxt->interval = endpoint->service_interval;
          cur_endpoint_ctxt->max_esit_payload_hi = 0;
          cur_endpoint_ctxt->error_count = 0;
          cur_endpoint_ctxt->endpoint_type = in_direction ? EP_TYPES::INTERRUPT_IN : EP_TYPES::INTERRUPT_OUT;
          cur_endpoint_ctxt->host_initiate_disable = 0;
          cur_endpoint_ctxt->max_burst_size = 0;
          cur_endpoint_ctxt->max_packet_size = endpoint->max_packet_size;
          cur_endpoint_ctxt->tr_dequeue_phys_ptr = reinterpret_cast<uint64_t>(
                                                   transfer_rings[addr][in_direction ? 1:0]->get_phys_base_address());
          cur_endpoint_ctxt->dequeue_cycle_state = 1;
          cur_endpoint_ctxt->average_trb_length = endpoint->max_packet_size;
          cur_endpoint_ctxt->max_esit_payload_lo = 0;

          break;

        default:
          panic("Invalid USB endpoint transfer type.");
        }
        ASSERT(cur_endpoint_ctxt->tr_dequeue_phys_ptr != 0);
      }
    }

    result = this->parent->configure_endpoints(this,
                                               reinterpret_cast<uint64_t>(mem_get_phys_addr(dev_input_context.get())),
                                               this->slot_id);

  }

  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Configuration successful.\n");
    last_known_state = DEV_STATE::CONFIGURED;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Device configuration failed!\n");
    last_known_state = DEV_STATE::UNKNOWN;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool device_core::queue_transfer(uint8_t endpoint_num,
                                 bool is_inwards,
                                 std::shared_ptr<normal_transfer> transfer_item)
{
  bool result = true;
  uint16_t endpoint_id = endpoint_num * 2 + (is_inwards ? 1 : 0);
  uint64_t trb_phys_addr = 0;

  KL_TRC_ENTRY;

  if ((endpoint_id < 2) || (endpoint_id > 31))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Endpoint ID out of range\n");
    result = false;
  }

  klib_synch_spinlock_lock(current_transfers_lock);

  if(transfer_rings[endpoint_num][is_inwards ? 1 : 0])
  {
    result = transfer_rings[endpoint_num][is_inwards ? 1 : 0]->
      queue_regular_transfer(transfer_item->transfer_buffer.get(), transfer_item->buffer_size, trb_phys_addr);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Endpoint not configured\n");
    result = false;
  }

  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Transfer queued, add to responses list\n");
    ASSERT(!map_contains(current_transfers, trb_phys_addr));
    current_transfers.insert({trb_phys_addr, transfer_item});
  }

  klib_synch_spinlock_unlock(current_transfers_lock);

  // Ring doorbell of the relevant transfer ring.
  parent->ring_doorbell(this->slot_id, endpoint_id);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
