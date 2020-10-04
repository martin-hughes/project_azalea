#pragma once

#include "work_queue.h"

class non_queueing : public work::IWorkQueue
{
public:
    virtual ~non_queueing() = default;

    virtual void queue_message(std::shared_ptr<work::message_receiver> receiver,
                               std::unique_ptr<msg::root_msg> msg) override
    {
      ASSERT(receiver->message_queue.empty());
      ASSERT(receiver->ready_to_receive); // We can't cope with asynchronous requests in this synchronous "queue".
      receiver->message_queue.push(std::move(msg));
      receiver->process_next_message();
    };

    virtual void work_queue_one_loop() override { };
};
