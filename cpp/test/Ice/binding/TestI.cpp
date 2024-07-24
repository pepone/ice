//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "TestI.h"
#include "Ice/Ice.h"
#include "TestHelper.h"

using namespace std;
using namespace Ice;
using namespace Test;

RemoteCommunicatorI::RemoteCommunicatorI() : _nextPort(1) {}

optional<RemoteObjectAdapterPrx>
RemoteCommunicatorI::createObjectAdapter(string name, string endpts, const Current& current)
{
    CommunicatorPtr com = current.adapter->getCommunicator();
    const string defaultProtocol = com->getProperties()->getProperty("Ice.Default.Protocol");
    int retry = 5;
    while (true)
    {
        try
        {
            string endpoints = endpts;
            if (defaultProtocol != "bt")
            {
                if (endpoints.find("-p") == string::npos)
                {
                    endpoints = TestHelper::getTestEndpoint(com->getProperties(), _nextPort++, endpoints);
                }
            }
            com->getProperties()->setProperty(name + ".ThreadPool.Size", "1");
            ObjectAdapterPtr adapter = com->createObjectAdapterWithEndpoints(name, endpoints);
            return current.adapter->addWithUUID<RemoteObjectAdapterPrx>(make_shared<RemoteObjectAdapterI>(adapter));
        }
        catch (const SocketException&)
        {
            if (--retry == 0)
            {
                throw;
            }
        }
    }
}

void
RemoteCommunicatorI::deactivateObjectAdapter(optional<RemoteObjectAdapterPrx> adapter, const Current&)
{
    adapter->deactivate(); // Collocated call
}

void
RemoteCommunicatorI::shutdown(const Current& current)
{
    current.adapter->getCommunicator()->shutdown();
}

RemoteObjectAdapterI::RemoteObjectAdapterI(const ObjectAdapterPtr& adapter)
    : _adapter(adapter),
      _testIntf(_adapter->add<TestIntfPrx>(make_shared<TestI>(), stringToIdentity("test")))
{
    _adapter->activate();
}

optional<TestIntfPrx>
RemoteObjectAdapterI::getTestIntf(const Current&)
{
    return _testIntf;
}

void
RemoteObjectAdapterI::deactivate(const Current&)
{
    try
    {
        _adapter->destroy();
    }
    catch (const ObjectAdapterDeactivatedException&)
    {
    }
}

std::string
TestI::getAdapterName(const Current& current)
{
    return current.adapter->getName();
}
