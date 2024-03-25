//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <TestI.h>

TestI::TestI(Ice::PropertiesPtr properties) : _properties(std::move(properties)) {}

void
TestI::shutdown(const Ice::Current& current)
{
    current.adapter->getCommunicator()->shutdown();
}

std::string
TestI::getProperty(std::string name, const Ice::Current&)
{
    return _properties->getProperty(name);
}
