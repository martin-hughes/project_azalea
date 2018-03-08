/// @file Basic tests of the IRQ handling system.
///
/// Obviously this can't test that the low-level ASM part works properly, so these tests cover the adding and removing
/// of receivers, and that the list system generally works correctly.

// Known defects:
// - If the IrqHandling test fails midway through it won't tidy up properly and future tests may crash.

#include "gtest/gtest.h"
#include "devices/device_interface.h"
#include "processor/processor.h"
#include "processor/processor-int.h"

class TestIrqHandler : public IIrqReceiver
{
public:
  TestIrqHandler();
  virtual bool handle_irq(unsigned char irq_number);

  bool irq_fired;
  bool allow_continue;
};

TEST(ProcessorTests, IrqHandling)
{
  TestIrqHandler a;
  TestIrqHandler b;

  // Test that neither IRQ handler is executed. Mostly a test that nothing goes terribly wrong.
  proc_handle_irq(0);
  ASSERT_FALSE(a.irq_fired);
  ASSERT_FALSE(b.irq_fired);

  // Add a to one IRQ and b to another. Check that only the appropriate IRQ fires.
  proc_register_irq_handler(0, &a);
  proc_register_irq_handler(1, &b);

  proc_handle_irq(0);
  ASSERT_TRUE(a.irq_fired);
  a.irq_fired = false;
  ASSERT_FALSE(b.irq_fired);

  proc_handle_irq(1);
  ASSERT_TRUE(b.irq_fired);
  b.irq_fired = false;
  ASSERT_FALSE(a.irq_fired);

  // Remove a from IRQ 0 and add it to IRQ 1. Check both handlers fire.
  proc_unregister_irq_handler(0, &a);
  proc_register_irq_handler(1, &a);
  proc_handle_irq(1);

  ASSERT_TRUE(a.irq_fired);
  ASSERT_TRUE(b.irq_fired);
  a.irq_fired = false;
  b.irq_fired = false;

  // Now check that the termination works properly
  b.allow_continue = false;
  proc_handle_irq(1);
  ASSERT_TRUE(b.irq_fired);
  b.irq_fired = false;
  ASSERT_FALSE(a.irq_fired);

  // Finally, tidy up.
  proc_unregister_irq_handler(1, &a);
  proc_unregister_irq_handler(1, &b);
}

TestIrqHandler::TestIrqHandler() : irq_fired(false), allow_continue(true) {}

bool TestIrqHandler::handle_irq(unsigned char irq_number)
{
  this->irq_fired = true;
  return this->allow_continue;
}