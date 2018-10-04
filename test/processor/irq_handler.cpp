/// @file Basic tests of the interrupt handling system.
///
/// Obviously this can't test that the low-level ASM part works properly, so these tests cover the adding and removing
/// of receivers, and that the list system generally works correctly.
///
/// Despite the name, this actually tests the interrupt handling system. Originally, drivers could only request to
/// handle IRQs and not 'true' interrupts - hence the name of the test.

// Known defects:
// - If the IrqHandling test fails midway through it won't tidy up properly and future tests may crash.

#include "test/test_core/test.h"
#include "gtest/gtest.h"
#include "devices/device_interface.h"
#include "processor/processor.h"
#include "processor/processor-int.h"

class TestIrqHandler : public IInterruptReceiver
{
public:
  TestIrqHandler();
  virtual bool handle_interrupt_fast(uint8_t irq_number) override;
  virtual void handle_interrupt_slow(uint8_t irq_number) override;

  bool irq_fired;
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

  // Finally, tidy up.
  proc_unregister_irq_handler(1, &a);
  proc_unregister_irq_handler(1, &b);
}

TestIrqHandler::TestIrqHandler() : irq_fired(false) {}

bool TestIrqHandler::handle_interrupt_fast(uint8_t irq_number)
{
  this->irq_fired = true;
  return false;
}

// There's no test that the slow path IRQ handler gets fired, sadly.
void TestIrqHandler::handle_interrupt_slow(uint8_t irq_number)
{

}