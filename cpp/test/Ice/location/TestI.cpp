//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "TestI.h"
#include "Ice/Ice.h"
#include "Ice/Locator.h"
#include "TestHelper.h"

using namespace std;
using namespace Ice;
using namespace Test;

ServerManagerI::ServerManagerI(const ServerLocatorRegistryPtr& registry, const InitializationData& initData)
    : _registry(registry),
      _initData(initData),
      _nextPort(1)
{
    _initData.properties->setProperty("TestAdapter.AdapterId", "TestAdapter");
    _initData.properties->setProperty("TestAdapter.ReplicaGroupId", "ReplicatedAdapter");
    _initData.properties->setProperty("TestAdapter2.AdapterId", "TestAdapter2");
    _initData.properties->setProperty("Ice.PrintAdapterReady", "0");
}

void
ServerManagerI::startServer(const Current&)
{
    for (const auto& communicator : _communicators)
    {
        communicator->waitForShutdown();
        communicator->destroy();
    }
    _communicators.clear();

    //
    // Simulate a server: create a new communicator and object
    // adapter. The object adapter is started on a system allocated
    // port. The configuration used here contains the Ice.Locator
    // configuration variable. The new object adapter will register
    // its endpoints with the locator and create references containing
    // the adapter id instead of the endpoints.
    //
    CommunicatorPtr serverCommunicator = initialize(_initData);
    _communicators.push_back(serverCommunicator);

    //
    // Use fixed port to ensure that OA re-activation doesn't re-use previous port from
    // another OA (e.g.: TestAdapter2 is re-activated using port of TestAdapter).
    //
    int nRetry = 10;
    while (--nRetry > 0)
    {
        ObjectAdapterPtr adapter;
        ObjectAdapterPtr adapter2;
        try
        {
            PropertiesPtr props = _initData.properties;
            serverCommunicator->getProperties()->setProperty(
                "TestAdapter.Endpoints",
                TestHelper::getTestEndpoint(props, _nextPort++));
            serverCommunicator->getProperties()->setProperty(
                "TestAdapter2.Endpoints",
                TestHelper::getTestEndpoint(props, _nextPort++));

            adapter = serverCommunicator->createObjectAdapter("TestAdapter");
            adapter2 = serverCommunicator->createObjectAdapter("TestAdapter2");

            LocatorPrx locator(serverCommunicator, "locator:" + TestHelper::getTestEndpoint(props));
            adapter->setLocator(locator);
            adapter2->setLocator(locator);

            ObjectPtr object = make_shared<TestI>(adapter, adapter2, _registry);
            _registry->addObject(adapter->add(object, stringToIdentity("test")));
            _registry->addObject(adapter->add(object, stringToIdentity("test2")));
            adapter->add(object, stringToIdentity("test3"));

            adapter->activate();
            adapter2->activate();
            break;
        }
        catch (const SocketException&)
        {
            if (nRetry == 0)
            {
                throw;
            }

            // Retry, if OA creation fails with EADDRINUSE (this can occur when running with JS web
            // browser clients if the driver uses ports in the same range as this test, ICE-8148)
            if (adapter)
            {
                adapter->destroy();
            }
            if (adapter2)
            {
                adapter2->destroy();
            }
        }
    }
}

void
ServerManagerI::shutdown(const Current& current)
{
    for (const auto& communicator : _communicators)
    {
        communicator->destroy();
    }
    current.adapter->getCommunicator()->shutdown();
}

TestI::TestI(
    const ObjectAdapterPtr& adapter,
    const ObjectAdapterPtr& adapter2,
    const ServerLocatorRegistryPtr& registry)
    : _adapter1(adapter),
      _adapter2(adapter2),
      _registry(registry)
{
    _registry->addObject(_adapter1->add(make_shared<HelloI>(), stringToIdentity("hello")));
}

void
TestI::shutdown(const Current&)
{
    _adapter1->getCommunicator()->shutdown();
}

optional<HelloPrx>
TestI::getHello(const Current&)
{
    return _adapter1->createIndirectProxy<HelloPrx>(stringToIdentity("hello"));
}

optional<HelloPrx>
TestI::getReplicatedHello(const Current&)
{
    return _adapter1->createProxy<HelloPrx>(stringToIdentity("hello"));
}

void
TestI::migrateHello(const Current&)
{
    const Identity id = stringToIdentity("hello");
    try
    {
        _registry->addObject(_adapter2->add(_adapter1->remove(id), id));
    }
    catch (const NotRegisteredException&)
    {
        _registry->addObject(_adapter1->add(_adapter2->remove(id), id));
    }
}

void
HelloI::sayHello(const Current&)
{
}
