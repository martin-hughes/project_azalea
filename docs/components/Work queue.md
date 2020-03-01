# Project Azalea work queue system

The basic idea of the work queue system is to allow one object to send a message to another object, for handling
asynchronously.

## Contents

- [Overview](#overview)
- [The work queue thread](#work-queue-thread)
- [Sending messages](#sending-messages)
- [Handling messages](#handling-messages)
- [Types of message](#types-of-message)
- [Receiving a reply](#receiving-a-reply)
- [Future improvements](#future-improvements)

## Overview

Asynchronous work is (partly, for now) handling using the "work queue". This is a dedicated thread that takes
"messages" sent by one thread and passes them to receiving objects to be handled.

Messages can be objects of any type that inherit from a base class (`msg::root_msg`). Receivers must inherit from
`work::message_receiver`.

At present, the work queue thread is a single thread that processes a single receiver at a time, passing it messages in
the order they were received. There is scope for extending this, for example to multiple threads or to allow
prioritisation of messages. This is a deliberate design decision - it allows any code invoked by the handler to be
assume that it is single threaded. Care is needed if members of the recipient class may be called simultaneously by
other threads, of course.

## Work queue thread

The work queue thread is implemented in `work::work_queue_thread()`. It is a single thread that iterates over all
receiver objects with messages in their queues, providing them messages to handle in the order that they were received.

Because it is a single thread, it is important that receiver objects do not block for extended periods. (Short blocks,
like waiting for the memory manager to allocate memory, are acceptable - waiting for a response from a network device
is not)

## Sending messages

To send a message, construct a `unique_ptr` to a class deriving from `msg::root_msg`. Pass this and a `shared_ptr` to
the receiver to `work::queue_message()`.

The work queue will queue the message and deal with it in sequence.

The caller *must* not edit the message after it is queue. The transfer in ownership is represented by the passing of a
`unique_ptr`.

## Handling messages

To become a message receiver, a class must derive from `work::message_receiver` and override the `handle_message()`
function.

While inside of `handle_message()` the object *must not* block for extended periods. (Short blocks, like waiting for
the memory manager to allocate memory, are acceptable - waiting for a response from a network device is not)

Inside of `handle_message()` the recipient owns the message object. It is discarded by the caller after
`handle_message()` completes, so it can be manipulated in any way desired.

If base classes also handle messages, it may be desirable for the child class to call base class versions of
`handle_message()` - this is not done automatically.

## Types of message

Messages can be any type that derives from `msg::root_msg`. They all contain a message ID field that helps identify the
message being send. However, the handling object may only permit certain message IDs to correspond to certain object
types.

## Receiving a reply

It is often useful to receive a notification of some kind that a message has been handled. There are three
possibilities:

1. Add a reference to a `syscall_semaphore_obj` object to the message. The message handling system will ensure that
   this semaphore is signalled when the message has been handled. Alternatively, if `auto_signal_semaphore` is set to
   false then the receiver can take charge of signalling that semaphore - for example, if it forwards the message to
   another handler.

2. All message types contain a provision for an 'output' buffer. This is space for the handler to put output in without
   needing to send a message back to the original sender. For example, it may be used to output properties of an
   object, or similar.

3. The message could be coded to contain details of the sending object, in which case the recipient can simply send a
   new message back to the caller.

## Future improvements

Here are some possible future improvements:

- Multiple work queue threads
- The option to allow one object to handle messages across multiple threads while retaining single-threaded behaviour
  for objects that desire it.
