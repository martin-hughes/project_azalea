# Object Manager

The Object Manager is a place where the kernel can store object references, and correlate them with handles that can be
used internally or by user-mode processes.

The Object Manager does not manage object lifetimes. It is the responsibility of the user to do that.

## Main Interface

The interface is fairly straightforward.

- `om_store_object` and `om_remove_object` add and remove objects to OM as needed. They also allocate and deallocate
  the associated handle.
- `om_correlate_object` and `om_decorrelate_object` do the same, but leave handle management up to the user.
- `om_retrieve_object` retrieves the object associated with a given handle.

There is no way to go lookup a handle given an object to search with.

## Algorithm

At the moment, the handle manager operates using a terrible algorithm. All stored objects are stored in a linked list,
which is search from beginning to end to look for the correct handle when needed.

It will be replaced!