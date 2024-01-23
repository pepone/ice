//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <TestSuite.h>
#include <ThreadPriority.h>
#include <TimerPriority.h>

std::list<TestBasePtr> allTests;

void
initializeTestSuite()
{
    allTests.push_back(new ThreadPriorityTest);
    allTests.push_back(new TimerPriorityTest);
}
