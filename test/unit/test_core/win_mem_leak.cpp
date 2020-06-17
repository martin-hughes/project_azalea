#ifdef UT_MEM_LEAK_CHECK

#include "test_core/test.h"
#include "gtest/gtest.h"
#include "win_mem_leak.h"

#include <iostream>

// This code is based heavily on that in the blog post:
// https://coffeepluscode.wordpress.com/2012/07/03/adding-memory-leak-checking-to-googletest-on-windows/

int __cdecl printToStdErr(int reportType, char* szMsg, int* retVal)
{
 std::cerr << szMsg;
 return 1; // No further processing required by _CrtDebugReport
}

void MemLeakListener::OnTestStart(const ::testing::TestInfo& test_info)
{
  _CrtMemCheckpoint( &memAtStart );
}

void MemLeakListener::OnTestEnd(const ::testing::TestInfo& test_info)
{
  if(test_info.result()->Passed())
  {
    CheckForMemLeaks(test_info);
  }
}

void MemLeakListener::OnTestProgramStart(const ::testing::UnitTest& unit_test)
{
  _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, &printToStdErr);
}

void MemLeakListener::OnTestProgramEnd(const ::testing::UnitTest& unit_test)
{
  _CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, &printToStdErr);
}

void MemLeakListener::CheckForMemLeaks(const ::testing::TestInfo& test_info)
{
  _CrtMemState memAtEnd;
  _CrtMemCheckpoint( &memAtEnd );
  _CrtMemState memDiff;
  if ( _CrtMemDifference( &memDiff, &memAtStart, &memAtEnd))
  {
    _CrtMemDumpStatistics( &memDiff );
    _CrtMemDumpAllObjectsSince(&memAtStart);
    FAIL() << "Memory leak in " << test_info.test_case_name() << "." << test_info.name() << '\n';
  }
}

#endif
