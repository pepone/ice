//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Instance.h"
#include "CallbackExecutor.h"
#include "ConnectionManager.h"
#include "DataStorm/Node.h"
#include "LookupI.h"
#include "NodeI.h"
#include "NodeSessionManager.h"
#include "TopicFactoryI.h"
#include "TraceUtil.h"

using namespace std;
using namespace DataStormI;

Instance::Instance(const Ice::CommunicatorPtr& communicator) : _communicator(communicator), _shutdown(false)
{
    Ice::PropertiesPtr properties = _communicator->getProperties();

    if (properties->getIcePropertyAsInt("DataStorm.Node.Server.Enabled") > 0)
    {
        if (properties->getIceProperty("DataStorm.Node.Server.Endpoints").empty())
        {
            properties->setProperty("DataStorm.Node.Server.Endpoints", "tcp");
        }
        properties->setProperty("DataStorm.Node.Server.ThreadPool.SizeMax", "1");

        try
        {
            _adapter = _communicator->createObjectAdapter("DataStorm.Node.Server");
        }
        catch (const Ice::LocalException& ex)
        {
            ostringstream os;
            os << "failed to listen on server endpoints `";
            os << properties->getIceProperty("DataStorm.Node.Server.Endpoints") << "':\n";
            os << ex.what();
            throw invalid_argument(os.str());
        }
    }
    else
    {
        _adapter = _communicator->createObjectAdapter("");
    }

    if (properties->getIcePropertyAsInt("DataStorm.Node.Multicast.Enabled") > 0)
    {
        if (properties->getIceProperty("DataStorm.Node.Multicast.Endpoints").empty())
        {
            properties->setProperty("DataStorm.Node.Multicast.Endpoints", "udp -h 239.255.0.1 -p 10000");
            // Set the published host to the multicast address, ensuring that proxies are created with the multicast
            // address.
            properties->setProperty("DataStorm.Node.Multicast.PublishedHost", "239.255.0.1");
            properties->setProperty("DataStorm.Node.Multicast.ProxyOptions", "-d");
        }
        properties->setProperty("DataStorm.Node.Multicast.ThreadPool.SizeMax", "1");

        try
        {
            _multicastAdapter = _communicator->createObjectAdapter("DataStorm.Node.Multicast");
        }
        catch (const Ice::LocalException& ex)
        {
            ostringstream os;
            os << "failed to listen on multicast endpoints `";
            os << properties->getIceProperty("DataStorm.Node.Multicast.Endpoints") << "':\n";
            os << ex.what();
            throw invalid_argument(os.str());
        }
    }

    _retryDelay = chrono::milliseconds(properties->getIcePropertyAsInt("DataStorm.Node.RetryDelay"));
    _retryMultiplier = properties->getIcePropertyAsInt("DataStorm.Node.RetryMultiplier");
    _retryCount = properties->getIcePropertyAsInt("DataStorm.Node.RetryCount");

    //
    // Create a collocated object adapter with a random name to prevent user configuration
    // of the adapter.
    //
    auto collocated = Ice::generateUUID();
    properties->setProperty(collocated + ".AdapterId", collocated);
    _collocatedAdapter = _communicator->createObjectAdapter(collocated);

    _collocatedForwarder = make_shared<ForwarderManager>(_collocatedAdapter, "forwarders");
    _collocatedAdapter->addDefaultServant(_collocatedForwarder, "forwarders");

    _executor = make_shared<CallbackExecutor>();
    _connectionManager = make_shared<ConnectionManager>(_executor);
    _timer = make_shared<IceInternal::Timer>();
    _traceLevels = make_shared<TraceLevels>(properties, _communicator->getLogger());
}

void
Instance::init()
{
    auto self = shared_from_this();

    _topicFactory = make_shared<TopicFactoryI>(self);

    _node = make_shared<NodeI>(self);
    _node->init();

    _nodeSessionManager = make_shared<NodeSessionManager>(self, _node);
    _nodeSessionManager->init();

    auto lookupI = make_shared<LookupI>(_nodeSessionManager, _topicFactory, _node->getProxy());
    _adapter->add(lookupI, {"Lookup", "DataStorm"});
    if (_multicastAdapter)
    {
        auto lookup = _multicastAdapter->add<DataStormContract::LookupPrx>(lookupI, {"Lookup", "DataStorm"});
        _lookup = lookup->ice_collocationOptimized(false);
    }

    _adapter->activate();
    _collocatedAdapter->activate();
    if (_multicastAdapter)
    {
        _multicastAdapter->activate();
    }
}

void
Instance::shutdown()
{
    unique_lock<mutex> lock(_mutex);
    _shutdown = true;
    _cond.notify_all();
    _topicFactory->shutdown();
}

bool
Instance::isShutdown() const
{
    unique_lock<mutex> lock(_mutex);
    return _shutdown;
}

void
Instance::checkShutdown() const
{
    unique_lock<mutex> lock(_mutex);
    if (_shutdown)
    {
        throw DataStorm::NodeShutdownException();
    }
}

void
Instance::waitForShutdown() const
{
    unique_lock<mutex> lock(_mutex);
    _cond.wait(lock, [&]() { return _shutdown; }); // Wait until shutdown is called
}

void
Instance::destroy(bool ownsCommunicator)
{
    if (ownsCommunicator)
    {
        _communicator->destroy();
    }
    else
    {
        _adapter->destroy();
        _collocatedAdapter->destroy();
        if (_multicastAdapter)
        {
            _multicastAdapter->destroy();
        }
    }
    _node->destroy(ownsCommunicator);

    _executor->destroy();
    _connectionManager->destroy();
    _collocatedForwarder->destroy();
    // Destroy the session manager before the timer to avoid scheduling new tasks after the timer has been destroyed.
    _nodeSessionManager->destroy();
    _timer->destroy();
}
