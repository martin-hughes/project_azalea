# Handle Manager subcomponent

The handle manager is part of the Object Manager component. It is responsible for keeping track of handles used
throughout the system. It is not responsible for correlating them with objects - that is the responsibility of other
parts of main object manager.

## Main Interface

There are two primary function calls:

- `hm_get_handle` provides a unique handle to the caller
- `hm_release_handle` releases the handle. It may then be reused, so the caller should not use it again itself.

## Algorithm

At the moment, the handle manager operates using the most simple of algorithms. There is an internal counter, which is
incremented whenever a new handle is requested. The value of the counter is used as the handle. When the counter
reaches its maximum value, the system has run out of handles and crashes.
