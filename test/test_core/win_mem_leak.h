#pragma once

#ifdef UT_MEM_LEAK_CHECK
class MemLeakListener : public ::testing::EmptyTestEventListener
{
public:
  virtual void OnTestStart(const ::testing::TestInfo& test_info) override;
  virtual void OnTestEnd(const ::testing::TestInfo& test_info) override;
  virtual void OnTestProgramStart(const ::testing::UnitTest& unit_test) override;
  virtual void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override;

private: 
  void CheckForMemLeaks(const ::testing::TestInfo& test_info);

  _CrtMemState memAtStart;
};

#endif