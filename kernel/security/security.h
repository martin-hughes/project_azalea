#ifndef __SECURITY_H
#define __SECURITY_H

// Structures and functions for supporting the kernel's security policy.
// Such as it is, right now...

// Security descriptor describing a process. Care should be taken to duplicate these, so that only one process has each
// instance at a time, otherwise editing one will cause another to change.
/*struct sec_process_info
{
  // Determines whether the process runs in kernel mode or not.
  bool kernel_mode;
};*/

#endif
