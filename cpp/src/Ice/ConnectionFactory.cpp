//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/ConnectionFactory.h>
#include <Ice/ConnectionI.h>
#include <Ice/Instance.h>
#include <Ice/LoggerUtil.h>
#include <Ice/TraceLevels.h>
#include <Ice/DefaultsAndOverrides.h>
#include <Ice/Properties.h>
#include <Ice/Transceiver.h>
#include <Ice/Connector.h>
#include <Ice/Acceptor.h>
#include <Ice/ThreadPool.h>
#include <Ice/ObjectAdapterI.h> // For getThreadPool().
#include <Ice/Reference.h>
#include <Ice/EndpointI.h>
#include <Ice/RouterInfo.h>
#include <Ice/LocalException.h>
#include <Ice/OutgoingAsync.h>
#include <Ice/CommunicatorI.h>
#include <IceUtil/Random.h>
#include <iterator>

#if TARGET_OS_IPHONE != 0
namespace IceInternal
{

bool registerForBackgroundNotification(const IceInternal::IncomingConnectionFactoryPtr&);
void unregisterForBackgroundNotification(const IceInternal::IncomingConnectionFactoryPtr&);

}
#endif

using namespace std;
using namespace Ice;
using namespace Ice::Instrumentation;
using namespace IceInternal;

namespace
{

template <typename Map> void
remove(Map& m, const typename Map::key_type& k, const typename Map::mapped_type& v)
{
    auto pr = m.equal_range(k);
    assert(pr.first != pr.second);
    for(auto q = pr.first; q != pr.second; ++q)
    {
        if(q->second.get() == v.get())
        {
            m.erase(q);
            return;
        }
    }
    assert(false); // Nothing was removed which is an error.
}

template<typename Map, typename Predicate> typename Map::mapped_type
find(const Map& m, const typename Map::key_type& k, Predicate predicate)
{
    auto pr = m.equal_range(k);
    for(auto q = pr.first; q != pr.second; ++q)
    {
        if(predicate(q->second))
        {
            return q->second;
        }
    }
    return nullptr;
}

class StartAcceptor : public IceUtil::TimerTask, public std::enable_shared_from_this<StartAcceptor>
{
public:

    StartAcceptor(const IncomingConnectionFactoryPtr& factory, const InstancePtr& instance) :
        _factory(factory), _instance(instance)
    {
    }

    void
    runTimerTask()
    {
        try
        {
            _factory->startAcceptor();
        }
        catch(const Ice::Exception& ex)
        {
            Error out(_instance->initializationData().logger);
            out << "acceptor creation failed:\n" << ex << '\n' << _factory->toString();
            _instance->timer()->schedule(shared_from_this(), IceUtil::Time::seconds(1));
        }
    }

private:

    IncomingConnectionFactoryPtr _factory;
    InstancePtr _instance;
};

#if TARGET_OS_IPHONE != 0
class FinishCall : public DispatchWorkItem
{
public:

    FinishCall(const IncomingConnectionFactoryPtr& factory) : _factory(factory)
    {
    }

    virtual void
    run()
    {
        _factory->finish();
    }

private:

    const IncomingConnectionFactoryPtr _factory;
};
#endif

}

bool
IceInternal::OutgoingConnectionFactory::ConnectorInfo::operator==(const ConnectorInfo& other) const
{
    return targetEqualTo(connector, other.connector);
}

void
IceInternal::OutgoingConnectionFactory::destroy()
{
    lock_guard lock(_mutex);

    if(_destroyed)
    {
        return;
    }

    for(const auto& p : _connections)
    {
        p.second->destroy(ConnectionI::CommunicatorDestroyed);
    }
    _destroyed = true;
    _communicator = nullptr;

    _conditionVariable.notify_all();
}

void
IceInternal::OutgoingConnectionFactory::updateConnectionObservers()
{
    lock_guard lock(_mutex);
    for(const auto& p : _connections)
    {
        p.second->updateObserver();
    }
}

void
IceInternal::OutgoingConnectionFactory::waitUntilFinished()
{
    multimap<ConnectorPtr, ConnectionIPtr, Ice::TargetCompare<ConnectorPtr, std::less>> connections;

    {
        unique_lock lock(_mutex);

        //
        // First we wait until the factory is destroyed. We also wait
        // until there are no pending connections anymore. Only then
        // we can be sure the _connections contains all connections.
        //
        _conditionVariable.wait(lock, [this] { return _destroyed && _pending.empty() && _pendingConnectCount == 0; });

        //
        // We want to wait until all connections are finished outside the
        // thread synchronization.
        //
        connections = _connections;
    }

    for(const auto& p : _connections)
    {
        p.second->waitUntilFinished();
    }

    {
        lock_guard lock(_mutex);
        // Ensure all the connections are finished and reapable at this point.
        vector<Ice::ConnectionIPtr> cons;
        _monitor->swapReapedConnections(cons);
        assert(cons.size() == _connections.size());
        cons.clear();
        _connections.clear();
        _connectionsByEndpoint.clear();
    }

    //
    // Must be destroyed outside the synchronization since this might block waiting for
    // a timer task to complete.
    //
    _monitor->destroy();
}

void
IceInternal::OutgoingConnectionFactory::create(const vector<EndpointIPtr>& endpts,
                                               bool hasMore,
                                               Ice::EndpointSelectionType selType,
                                               const CreateConnectionCallbackPtr& callback)
{
    assert(!endpts.empty());

    //
    // Apply the overrides.
    //
    vector<EndpointIPtr> endpoints = applyOverrides(endpts);

    //
    // Try to find a connection to one of the given endpoints.
    //
    try
    {
        bool compress;
        Ice::ConnectionIPtr connection = findConnection(endpoints, compress);
        if(connection)
        {
            callback->setConnection(connection, compress);
            return;
        }
    }
    catch (const std::exception&)
    {
        callback->setException(current_exception());
        return;
    }

    auto cb = make_shared<ConnectCallback>(_instance, shared_from_this(), endpoints, hasMore, callback, selType);
    cb->getConnectors();
}

void
IceInternal::OutgoingConnectionFactory::setRouterInfo(const RouterInfoPtr& routerInfo)
{
    assert(routerInfo);
    ObjectAdapterPtr adapter = routerInfo->getAdapter();
    vector<EndpointIPtr> endpoints = routerInfo->getClientEndpoints(); // Must be called outside the synchronization

    lock_guard lock(_mutex);

    if(_destroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    //
    // Search for connections to the router's client proxy endpoints,
    // and update the object adapter for such connections, so that
    // callbacks from the router can be received over such
    // connections.
    //
    for(vector<EndpointIPtr>::const_iterator p = endpoints.begin(); p != endpoints.end(); ++p)
    {
        EndpointIPtr endpoint = *p;

        //
        // Modify endpoints with overrides.
        //
        if(_instance->defaultsAndOverrides()->overrideTimeout)
        {
            endpoint = endpoint->timeout(_instance->defaultsAndOverrides()->overrideTimeoutValue);
        }

        //
        // The Connection object does not take the compression flag of
        // endpoints into account, but instead gets the information
        // about whether messages should be compressed or not from
        // other sources. In order to allow connection sharing for
        // endpoints that differ in the value of the compression flag
        // only, we always set the compression flag to false here in
        // this connection factory.
        //
        endpoint = endpoint->compress(false);

        for(multimap<ConnectorPtr, ConnectionIPtr>::const_iterator q = _connections.begin();
            q != _connections.end(); ++q)
        {
            if(q->second->endpoint() == endpoint)
            {
                q->second->setAdapter(adapter);
            }
        }
    }
}

void
IceInternal::OutgoingConnectionFactory::removeAdapter(const ObjectAdapterPtr& adapter)
{
    lock_guard lock(_mutex);

    if(_destroyed)
    {
        return;
    }

    for(multimap<ConnectorPtr, ConnectionIPtr>::const_iterator p = _connections.begin(); p != _connections.end(); ++p)
    {
        if(p->second->getAdapter() == adapter)
        {
            p->second->setAdapter(0);
        }
    }
}

void
IceInternal::OutgoingConnectionFactory::flushAsyncBatchRequests(const CommunicatorFlushBatchAsyncPtr& outAsync,
                                                                Ice::CompressBatch compress)
{
    list<ConnectionIPtr> c;

    {
        lock_guard lock(_mutex);
        for(multimap<ConnectorPtr,  ConnectionIPtr>::const_iterator p = _connections.begin(); p != _connections.end();
            ++p)
        {
            if(p->second->isActiveOrHolding())
            {
                c.push_back(p->second);
            }
        }
    }

    for(list<ConnectionIPtr>::const_iterator p = c.begin(); p != c.end(); ++p)
    {
        try
        {
            outAsync->flushConnection(*p, compress);
        }
        catch(const LocalException&)
        {
            // Ignore.
        }
    }
}

IceInternal::OutgoingConnectionFactory::OutgoingConnectionFactory(const CommunicatorPtr& communicator,
                                                                  const InstancePtr& instance) :
    _communicator(communicator),
    _instance(instance),
    _monitor(new FactoryACMMonitor(instance, instance->clientACM())),
    _destroyed(false),
    _pendingConnectCount(0)
{
}

IceInternal::OutgoingConnectionFactory::~OutgoingConnectionFactory()
{
    assert(_destroyed);
    assert(_connections.empty());
    assert(_connectionsByEndpoint.empty());
    assert(_pending.empty());
    assert(_pendingConnectCount == 0);
}

vector<EndpointIPtr>
IceInternal::OutgoingConnectionFactory::applyOverrides(const vector<EndpointIPtr>& endpts)
{
    DefaultsAndOverridesPtr defaultsAndOverrides = _instance->defaultsAndOverrides();
    vector<EndpointIPtr> endpoints = endpts;
    for(vector<EndpointIPtr>::iterator p = endpoints.begin(); p != endpoints.end(); ++p)
    {
        //
        // Modify endpoints with overrides.
        //
        if(defaultsAndOverrides->overrideTimeout)
        {
            *p = (*p)->timeout(defaultsAndOverrides->overrideTimeoutValue);
        }
    }
    return endpoints;
}

ConnectionIPtr
IceInternal::OutgoingConnectionFactory::findConnection(const vector<EndpointIPtr>& endpoints, bool& compress)
{
    lock_guard lock(_mutex);
    if(_destroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    DefaultsAndOverridesPtr defaultsAndOverrides = _instance->defaultsAndOverrides();
    assert(!endpoints.empty());

    for(const auto& p : endpoints)
    {
        auto connection = find(_connectionsByEndpoint,
                               p,
                               [](const ConnectionIPtr& conn)
                               {
                                   return conn->isActiveOrHolding();
                               });
        if(connection)
        {
            if(defaultsAndOverrides->overrideCompress)
            {
                compress = defaultsAndOverrides->overrideCompressValue;
            }
            else
            {
                compress = p->compress();
            }
            return connection;
        }
    }
    return 0;
}

ConnectionIPtr
IceInternal::OutgoingConnectionFactory::findConnection(const vector<ConnectorInfo>& connectors, bool& compress)
{
    // This must be called with the mutex locked.

    DefaultsAndOverridesPtr defaultsAndOverrides = _instance->defaultsAndOverrides();
    for(const auto& p : connectors)
    {
        if(_pending.find(p.connector) != _pending.end())
        {
            continue;
        }

        auto connection = find(_connections,
                               p.connector,
                               [](const ConnectionIPtr& conn)
                               {
                                   return conn->isActiveOrHolding();
                               });
        if(connection)
        {
            if(defaultsAndOverrides->overrideCompress)
            {
                compress = defaultsAndOverrides->overrideCompressValue;
            }
            else
            {
                    compress = p.endpoint->compress();
            }
            return connection;
        }
    }

    return 0;
}

void
IceInternal::OutgoingConnectionFactory::incPendingConnectCount()
{
    //
    // Keep track of the number of pending connects. The outgoing connection factory
    // waitUntilFinished() method waits for all the pending connects to terminate before
    // to return. This ensures that the communicator client thread pool isn't destroyed
    // too soon and will still be available to execute the ice_exception() callbacks for
    // the asynchronous requests waiting on a connection to be established.
    //

    lock_guard lock(_mutex);
    if(_destroyed)
    {
        throw Ice::CommunicatorDestroyedException(__FILE__, __LINE__);
    }
    ++_pendingConnectCount;
}

void
IceInternal::OutgoingConnectionFactory::decPendingConnectCount()
{
    lock_guard lock(_mutex);
    --_pendingConnectCount;
    assert(_pendingConnectCount >= 0);
    if(_destroyed && _pendingConnectCount == 0)
    {
        _conditionVariable.notify_all();
    }
}

ConnectionIPtr
IceInternal::OutgoingConnectionFactory::getConnection(const vector<ConnectorInfo>& connectors,
                                                      const ConnectCallbackPtr& cb,
                                                      bool& compress)
{
    {
        unique_lock lock(_mutex);
        if(_destroyed)
        {
            throw Ice::CommunicatorDestroyedException(__FILE__, __LINE__);
        }

        //
        // Reap closed connections
        //
        vector<Ice::ConnectionIPtr> cons;
        _monitor->swapReapedConnections(cons);
        for(const auto& p : cons)
        {
            remove(_connections, p->connector(), p);
            remove(_connectionsByEndpoint, p->endpoint(), p);
            remove(_connectionsByEndpoint, p->endpoint()->compress(true), p);
        }

        //
        // Try to get the connection. We may need to wait for other threads to
        // finish if one of them is currently establishing a connection to one
        // of our connectors.
        //
        while(true)
        {
            if(_destroyed)
            {
                throw Ice::CommunicatorDestroyedException(__FILE__, __LINE__);
            }

            //
            // Search for a matching connection. If we find one, we're done.
            //
            Ice::ConnectionIPtr connection = findConnection(connectors, compress);
            if(connection)
            {
                return connection;
            }

            //
            // Determine whether another thread/request is currently attempting to connect to
            // one of our endpoints; if so we wait until it's done.
            //
            if(addToPending(cb, connectors))
            {
                //
                // If a callback is not specified we wait until another thread notifies us about a
                // change to the pending list. Otherwise, if a callback is provided we're done:
                // when the pending list changes the callback will be notified and will try to
                // get the connection again.
                //
                if(!cb)
                {
                    _conditionVariable.wait(lock);
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                //
                // If no thread is currently establishing a connection to one of our connectors,
                // we get out of this loop and start the connection establishment to one of the
                // given connectors.
                //
                break;
            }
        }
    }

    //
    // At this point, we're responsible for establishing the connection to one of
    // the given connectors. If it's a non-blocking connect, calling nextConnector
    // will start the connection establishment. Otherwise, we return null to get
    // the caller to establish the connection.
    //
    if(cb)
    {
        cb->nextConnector();
    }

    return 0;
}

ConnectionIPtr
IceInternal::OutgoingConnectionFactory::createConnection(const TransceiverPtr& transceiver, const ConnectorInfo& ci)
{
    lock_guard lock(_mutex);
    assert(_pending.find(ci.connector) != _pending.end() && transceiver);

    //
    // Create and add the connection to the connection map. Adding the connection to the map
    // is necessary to support the interruption of the connection initialization and validation
    // in case the communicator is destroyed.
    //
    Ice::ConnectionIPtr connection;
    try
    {
        if(_destroyed)
        {
            throw Ice::CommunicatorDestroyedException(__FILE__, __LINE__);
        }

        connection = ConnectionI::create(_communicator, _instance, _monitor, transceiver, ci.connector,
                                         ci.endpoint->compress(false), nullptr);
    }
    catch(const Ice::LocalException&)
    {
        try
        {
            transceiver->close();
        }
        catch(const Ice::LocalException&)
        {
            // Ignore
        }
        throw;
    }

    _connections.insert(pair<const ConnectorPtr, ConnectionIPtr>(ci.connector, connection));
    _connectionsByEndpoint.insert(pair<const EndpointIPtr, ConnectionIPtr>(connection->endpoint(), connection));
    _connectionsByEndpoint.insert(pair<const EndpointIPtr, ConnectionIPtr>(connection->endpoint()->compress(true),
                                                                           connection));
    return connection;
}

void
IceInternal::OutgoingConnectionFactory::finishGetConnection(const vector<ConnectorInfo>& connectors,
                                                            const ConnectorInfo& ci,
                                                            const ConnectionIPtr& connection,
                                                            const ConnectCallbackPtr& cb)
{
    ConnectCallbackSet connectionCallbacks;
    if(cb)
    {
        connectionCallbacks.insert(cb);
    }

    ConnectCallbackSet callbacks;
    {
        lock_guard lock(_mutex);
        for(vector<ConnectorInfo>::const_iterator p = connectors.begin(); p != connectors.end(); ++p)
        {
            auto q = _pending.find(p->connector);
            if(q != _pending.end())
            {
                for(auto r = q->second.begin(); r != q->second.end(); ++r)
                {
                    if((*r)->hasConnector(ci))
                    {
                        connectionCallbacks.insert(*r);
                    }
                    else
                    {
                        callbacks.insert(*r);
                    }
                }
                _pending.erase(q);
            }
        }

        for(auto r = connectionCallbacks.begin(); r != connectionCallbacks.end(); ++r)
        {
            (*r)->removeFromPending();
            callbacks.erase(*r);
        }
        for(auto r = callbacks.begin(); r != callbacks.end(); ++r)
        {
            (*r)->removeFromPending();
        }
        _conditionVariable.notify_all();
    }

    bool compress;
    DefaultsAndOverridesPtr defaultsAndOverrides = _instance->defaultsAndOverrides();
    if(defaultsAndOverrides->overrideCompress)
    {
        compress = defaultsAndOverrides->overrideCompressValue;
    }
    else
    {
        compress = ci.endpoint->compress();
    }

    for(auto p = callbacks.begin(); p != callbacks.end(); ++p)
    {
        (*p)->getConnection();
    }
    for(auto p = connectionCallbacks.begin(); p != connectionCallbacks.end(); ++p)
    {
        (*p)->setConnection(connection, compress);
    }
}

void
IceInternal::OutgoingConnectionFactory::finishGetConnection(const vector<ConnectorInfo>& connectors,
                                                            std::exception_ptr ex,
                                                            const ConnectCallbackPtr& cb)
{
    ConnectCallbackSet failedCallbacks;
    if(cb)
    {
        failedCallbacks.insert(cb);
    }

    ConnectCallbackSet callbacks;
    {
        lock_guard lock(_mutex);
        for(auto p = connectors.begin(); p != connectors.end(); ++p)
        {
            auto q = _pending.find(p->connector);
            if(q != _pending.end())
            {
                for(auto r = q->second.begin(); r != q->second.end(); ++r)
                {
                    if((*r)->removeConnectors(connectors))
                    {
                        failedCallbacks.insert(*r);
                    }
                    else
                    {
                        callbacks.insert(*r);
                    }
                }
                _pending.erase(q);
            }
        }

        for(auto r = callbacks.begin(); r != callbacks.end(); ++r)
        {
            assert(failedCallbacks.find(*r) == failedCallbacks.end());
            (*r)->removeFromPending();
        }
        _conditionVariable.notify_all();
    }

    for(auto p = callbacks.begin(); p != callbacks.end(); ++p)
    {
        (*p)->getConnection();
    }
    for(auto p = failedCallbacks.begin(); p != failedCallbacks.end(); ++p)
    {
        (*p)->setException(ex);
    }
}

bool
IceInternal::OutgoingConnectionFactory::addToPending(const ConnectCallbackPtr& cb,
                                                     const vector<ConnectorInfo>& connectors)
{
    //
    // Add the callback to each connector pending list.
    //
    bool found = false;
    for(auto p = connectors.begin(); p != connectors.end(); ++p)
    {
        auto q = _pending.find(p->connector);
        if(q != _pending.end())
        {
            found = true;
            if(cb)
            {
                q->second.insert(cb);
            }
        }
    }

    if(found)
    {
        return true;
    }

    //
    // If there's no pending connection for the given connectors, we're
    // responsible for its establishment. We add empty pending lists,
    // other callbacks to the same connectors will be queued.
    //
    for(vector<ConnectorInfo>::const_iterator r = connectors.begin(); r != connectors.end(); ++r)
    {
        if(_pending.find(r->connector) == _pending.end())
        {
            _pending.insert(make_pair(r->connector, ConnectCallbackSet()));
        }
    }
    return false;
}

void
IceInternal::OutgoingConnectionFactory::removeFromPending(const ConnectCallbackPtr& cb,
                                                          const vector<ConnectorInfo>& connectors)
{
    for(auto p = connectors.begin(); p != connectors.end(); ++p)
    {
        auto q = _pending.find(p->connector);
        if(q != _pending.end())
        {
            q->second.erase(cb);
        }
    }
}

void
IceInternal::OutgoingConnectionFactory::handleException(exception_ptr ex, bool hasMore)
{
    TraceLevelsPtr traceLevels = _instance->traceLevels();
    if(traceLevels->network >= 2)
    {
        Trace out(_instance->initializationData().logger, traceLevels->networkCat);

        out << "couldn't resolve endpoint host";

        try
        {
            rethrow_exception(ex);
        }
        catch (const CommunicatorDestroyedException& e)
        {
            out << "\n" << e;
        }
        catch (const std::exception& e)
        {
            if(hasMore)
            {
                out << ", trying next endpoint\n";
            }
            else
            {
                out << " and no more endpoints to try\n";
            }
            out << e;
        }
    }
}

void
IceInternal::OutgoingConnectionFactory::handleConnectionException(exception_ptr ex, bool hasMore)
{
    TraceLevelsPtr traceLevels = _instance->traceLevels();
    if(traceLevels->network >= 2)
    {
        Trace out(_instance->initializationData().logger, traceLevels->networkCat);

        out << "connection to endpoint failed";

        try
        {
            rethrow_exception(ex);
        }
        catch (const CommunicatorDestroyedException& e)
        {
            out << "\n" << e;
        }
        catch (const std::exception& e)
        {
            if(hasMore)
            {
                out << ", trying next endpoint\n";
            }
            else
            {
                out << " and no more endpoints to try\n";
            }
            out << e;
        }
    }
}

IceInternal::OutgoingConnectionFactory::ConnectCallback::ConnectCallback(const InstancePtr& instance,
                                                                         const OutgoingConnectionFactoryPtr& factory,
                                                                         const vector<EndpointIPtr>& endpoints,
                                                                         bool hasMore,
                                                                         const CreateConnectionCallbackPtr& cb,
                                                                         Ice::EndpointSelectionType selType) :
    _instance(instance),
    _factory(factory),
    _endpoints(endpoints),
    _hasMore(hasMore),
    _callback(cb),
    _selType(selType)
{
    _endpointsIter = _endpoints.begin();
}

//
// Methods from ConnectionI.StartCallback
//
void
IceInternal::OutgoingConnectionFactory::ConnectCallback::connectionStartCompleted(const ConnectionIPtr& connection)
{
    if(_observer)
    {
        _observer->detach();
    }

    connection->activate();
    _factory->finishGetConnection(_connectors, *_iter, connection, shared_from_this());
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::connectionStartFailed(const ConnectionIPtr& /*connection*/,
                                                                               exception_ptr ex)
{
    assert(_iter != _connectors.end());
    if(connectionStartFailedImpl(ex))
    {
        nextConnector();
    }
}

//
// Methods from EndpointI_connectors
//
void
IceInternal::OutgoingConnectionFactory::ConnectCallback::connectors(const vector<ConnectorPtr>& connectors)
{
    for(vector<ConnectorPtr>::const_iterator p = connectors.begin(); p != connectors.end(); ++p)
    {
        _connectors.push_back(ConnectorInfo(*p, *_endpointsIter));
    }

    if(++_endpointsIter != _endpoints.end())
    {
        nextEndpoint();
    }
    else
    {
        assert(!_connectors.empty());

        //
        // We now have all the connectors for the given endpoints. We can try to obtain the
        // connection.
        //
        _iter = _connectors.begin();
        getConnection();
    }
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::exception(exception_ptr ex)
{
    _factory->handleException(ex, _hasMore || _endpointsIter != _endpoints.end() - 1);
    if(++_endpointsIter != _endpoints.end())
    {
        nextEndpoint();
    }
    else if(!_connectors.empty())
    {
        //
        // We now have all the connectors for the given endpoints. We can try to obtain the
        // connection.
        //
        _iter = _connectors.begin();
        getConnection();
    }
    else
    {
        _callback->setException(ex);
        _factory->decPendingConnectCount(); // Must be called last.
    }
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::getConnectors()
{
    try
    {
        //
        // Notify the factory that there's an async connect pending. This is necessary
        // to prevent the outgoing connection factory to be destroyed before all the
        // pending asynchronous connects are finished.
        //
        _factory->incPendingConnectCount();
    }
    catch (const std::exception&)
    {
        _callback->setException(current_exception());
        return;
    }

    nextEndpoint();
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::nextEndpoint()
{
    try
    {
        auto self = shared_from_this();
        assert(_endpointsIter != _endpoints.end());
        (*_endpointsIter)->connectorsAsync(
            _selType,
            [self](const vector<ConnectorPtr>& connectors)
            {
                self->connectors(connectors);
            },
            [self](exception_ptr ex)
            {
                self->exception(ex);
            });

    }
    catch (const std::exception&)
    {
        exception(current_exception());
    }
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::getConnection()
{
    try
    {
        //
        // If all the connectors have been created, we ask the factory to get a
        // connection.
        //
        bool compress;
        Ice::ConnectionIPtr connection = _factory->getConnection(_connectors, shared_from_this(), compress);
        if(!connection)
        {
            //
            // A null return value from getConnection indicates that the connection
            // is being established and that everthing has been done to ensure that
            // the callback will be notified when the connection establishment is
            // done or that the callback already obtain the connection.
            //
            return;
        }

        _callback->setConnection(connection, compress);
        _factory->decPendingConnectCount(); // Must be called last.
    }
    catch (const std::exception&)
    {
        _callback->setException(current_exception());
        _factory->decPendingConnectCount(); // Must be called last.
    }
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::nextConnector()
{
    while(true)
    {
        try
        {
            const CommunicatorObserverPtr& obsv = _factory->_instance->initializationData().observer;
            if(obsv)
            {
                _observer = obsv->getConnectionEstablishmentObserver(_iter->endpoint, _iter->connector->toString());
                if(_observer)
                {
                    _observer->attach();
                }
            }

            assert(_iter != _connectors.end());

            if(_instance->traceLevels()->network >= 2)
            {
                Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
                out << "trying to establish " << _iter->endpoint->protocol() << " connection to "
                    << _iter->connector->toString();
            }
            Ice::ConnectionIPtr connection = _factory->createConnection(_iter->connector->connect(), *_iter);
            auto self = shared_from_this();
            connection->start(
                [self](ConnectionIPtr conn)
                {
                    self->connectionStartCompleted(std::move(conn));
                },
                [self](ConnectionIPtr conn, exception_ptr ex)
                {
                    self->connectionStartFailed(std::move(conn), ex);
                });
        }
        catch(const Ice::LocalException& ex)
        {
            if(_instance->traceLevels()->network >= 2)
            {
                Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
                out << "failed to establish " << _iter->endpoint->protocol() << " connection to "
                    << _iter->connector->toString() << "\n" << ex;
            }

            if(connectionStartFailedImpl(current_exception()))
            {
                continue; // More connectors to try, continue.
            }
        }
        break;
    }
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::setConnection(const Ice::ConnectionIPtr& connection,
                                                                       bool compress)
{
    //
    // Callback from the factory: the connection to one of the callback
    // connectors has been established.
    //
    _callback->setConnection(connection, compress);
    _factory->decPendingConnectCount(); // Must be called last.
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::setException(exception_ptr ex)
{
    //
    // Callback from the factory: connection establishment failed.
    //
    _callback->setException(ex);
    _factory->decPendingConnectCount(); // Must be called last.
}

bool
IceInternal::OutgoingConnectionFactory::ConnectCallback::hasConnector(const ConnectorInfo& ci)
{
    return find(_connectors.begin(), _connectors.end(), ci) != _connectors.end();
}

bool
IceInternal::OutgoingConnectionFactory::ConnectCallback::removeConnectors(const vector<ConnectorInfo>& connectors)
{
    //
    // Callback from the factory: connecting to the given connectors
    // failed, we remove the connectors and return true if there's
    // no more connectors left to try.
    //
    for (const auto& p : connectors)
    {
        _connectors.erase(remove(_connectors.begin(), _connectors.end(), p), _connectors.end());
    }
    return _connectors.empty();
}

void
IceInternal::OutgoingConnectionFactory::ConnectCallback::removeFromPending()
{
    _factory->removeFromPending(shared_from_this(), _connectors);
}

bool
IceInternal::OutgoingConnectionFactory::ConnectCallback::operator<(const ConnectCallback& rhs) const
{
    return this < &rhs;
}

bool
IceInternal::OutgoingConnectionFactory::ConnectCallback::connectionStartFailedImpl(std::exception_ptr ex)
{
    bool communicatorDestroyed = false;
    try
    {
        rethrow_exception(ex);
    }
    catch (const CommunicatorDestroyedException&)
    {
        communicatorDestroyed = true;
    }
    catch (...)
    {
    }

    if(_observer)
    {
        _observer->failed(getExceptionId(ex));
        _observer->detach();
    }

    _factory->handleConnectionException(ex, _hasMore || _iter != _connectors.end() - 1);

    if(communicatorDestroyed) // No need to continue.
    {
        _factory->finishGetConnection(_connectors, ex, shared_from_this());
    }
    else if(++_iter != _connectors.end()) // Try the next connector.
    {
        return true;
    }
    else
    {
        _factory->finishGetConnection(_connectors, ex, shared_from_this());
    }
    return false;
}

void
IceInternal::IncomingConnectionFactory::activate()
{
    lock_guard lock(_mutex);
    setState(StateActive);
}

void
IceInternal::IncomingConnectionFactory::hold()
{
    lock_guard lock(_mutex);
    setState(StateHolding);
}

void
IceInternal::IncomingConnectionFactory::destroy()
{
    lock_guard lock(_mutex);
    setState(StateClosed);
}

void
IceInternal::IncomingConnectionFactory::updateConnectionObservers()
{
    lock_guard lock(_mutex);
    for (const auto& conn : _connections)
    {
        conn->updateObserver();
    }
}

void
IceInternal::IncomingConnectionFactory::waitUntilHolding() const
{
    set<ConnectionIPtr> connections;

    {
        unique_lock lock(_mutex);

        //
        // First we wait until the connection factory itself is in holding
        // state.
        //
        _conditionVariable.wait(lock, [this] { return _state >= StateHolding; });

        //
        // We want to wait until all connections are in holding state
        // outside the thread synchronization.
        //
        connections = _connections;
    }

    //
    // Now we wait until each connection is in holding state.
    //
    for(const auto& conn : connections)
    {
        conn->waitUntilHolding();
    }
}

void
IceInternal::IncomingConnectionFactory::waitUntilFinished()
{
    set<ConnectionIPtr> connections;
    {
        unique_lock lock(_mutex);

        //
        // First we wait until the factory is destroyed. If we are using
        // an acceptor, we also wait for it to be closed.
        //
        _conditionVariable.wait(lock, [this] { return _state == StateFinished; });

        //
        // Clear the OA. See bug 1673 for the details of why this is necessary.
        //
        _adapter = nullptr;

        // We want to wait until all connections are finished outside the
        // thread synchronization.
        //
        connections = _connections;
    }

    for(const auto& conn : connections)
    {
        conn->waitUntilFinished();
    }

    {
        lock_guard lock(_mutex);
        if(_transceiver)
        {
            assert(_connections.size() <= 1); // The connection isn't monitored or reaped.
        }
        else
        {
            // Ensure all the connections are finished and reapable at this point.
            vector<Ice::ConnectionIPtr> cons;
            _monitor->swapReapedConnections(cons);
            assert(cons.size() == _connections.size());
            cons.clear();
        }
        _connections.clear();
    }

    //
    // Must be destroyed outside the synchronization since this might block waiting for
    // a timer task to complete.
    //
    _monitor->destroy();
}

bool
IceInternal::IncomingConnectionFactory::isLocal(const EndpointIPtr& endpoint) const
{
    if(_publishedEndpoint && endpoint->equivalent(_publishedEndpoint))
    {
        return true;
    }
    lock_guard lock(_mutex);
    return endpoint->equivalent(_endpoint);
}

EndpointIPtr
IceInternal::IncomingConnectionFactory::endpoint() const
{
    if(_publishedEndpoint)
    {
        return _publishedEndpoint;
    }
    lock_guard lock(_mutex);
    return _endpoint;
}

list<ConnectionIPtr>
IceInternal::IncomingConnectionFactory::connections() const
{
    lock_guard lock(_mutex);

    list<ConnectionIPtr> result;

    //
    // Only copy connections which have not been destroyed.
    //
    remove_copy_if(_connections.begin(), _connections.end(), back_inserter(result),
                   [](const ConnectionIPtr& conn)
                   {
                       return !conn->isActiveOrHolding();
                   });
    return result;
}

void
IceInternal::IncomingConnectionFactory::flushAsyncBatchRequests(const CommunicatorFlushBatchAsyncPtr& outAsync,
                                                                Ice::CompressBatch compress)
{
    list<ConnectionIPtr> c = connections(); // connections() is synchronized, so no need to synchronize here.

    for(list<ConnectionIPtr>::const_iterator p = c.begin(); p != c.end(); ++p)
    {
        try
        {
            outAsync->flushConnection(*p, compress);
        }
        catch(const LocalException&)
        {
            // Ignore.
        }
    }
}

#if defined(ICE_USE_IOCP)
bool
IceInternal::IncomingConnectionFactory::startAsync(SocketOperation)
{
    assert(_acceptor);
    if(_state >= StateClosed)
    {
        return false;
    }

    try
    {
        _acceptor->startAccept();
    }
    catch(const Ice::LocalException&)
    {
        _acceptorException = current_exception();
        _acceptor->getNativeInfo()->completed(SocketOperationRead);
    }
    return true;
}

bool
IceInternal::IncomingConnectionFactory::finishAsync(SocketOperation)
{
    assert(_acceptor);
    try
    {
        if(_acceptorException)
        {
            rethrow_exception(_acceptorException);
        }
        _acceptor->finishAccept();
    }
    catch(const LocalException& ex)
    {
        _acceptorException = nullptr;

        Error out(_instance->initializationData().logger);
        out << "couldn't accept connection:\n" << ex << '\n' << _acceptor->toString();
        if(_acceptorStarted)
        {
            _acceptorStarted = false;
            if(_adapter->getThreadPool()->finish(shared_from_this(), true))
            {
                closeAcceptor();
            }
        }
    }
    return _state < StateClosed;
}
#endif

void
IceInternal::IncomingConnectionFactory::message(ThreadPoolCurrent& current)
{
    ConnectionIPtr connection;

    ThreadPoolMessage<IncomingConnectionFactory> msg(current, *this);

    {
        lock_guard lock(_mutex);

        ThreadPoolMessage<IncomingConnectionFactory>::IOScope io(msg);
        if(!io)
        {
            return;
        }

        if(_state >= StateClosed)
        {
            return;
        }
        else if(_state == StateHolding)
        {
            IceUtil::ThreadControl::yield();
            return;
        }

        //
        // Reap closed connections
        //
        vector<Ice::ConnectionIPtr> cons;
        _monitor->swapReapedConnections(cons);
        for(vector<Ice::ConnectionIPtr>::const_iterator p = cons.begin(); p != cons.end(); ++p)
        {
            _connections.erase(*p);
        }

        if(!_acceptorStarted)
        {
            return;
        }

        //
        // Now accept a new connection.
        //
        TransceiverPtr transceiver;
        try
        {
            transceiver = _acceptor->accept();

            if(_instance->traceLevels()->network >= 2)
            {
                Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
                out << "trying to accept " << _endpoint->protocol() << " connection\n" << transceiver->toString();
            }
        }
        catch(const SocketException& ex)
        {
            if(noMoreFds(ex.error))
            {
                Error out(_instance->initializationData().logger);
                out << "can't accept more connections:\n" << ex << '\n' << _acceptor->toString();

                assert(_acceptorStarted);
                _acceptorStarted = false;
                if(_adapter->getThreadPool()->finish(shared_from_this(), true))
                {
                    closeAcceptor();
                }
            }

            // Ignore socket exceptions.
            return;
        }
        catch(const LocalException& ex)
        {
            // Warn about other Ice local exceptions.
            if(_warn)
            {
                Warning out(_instance->initializationData().logger);
                out << "connection exception:\n" << ex << '\n' << _acceptor->toString();
            }
            return;
        }

        assert(transceiver);

        try
        {
            connection = ConnectionI::create(_adapter->getCommunicator(), _instance, _monitor, transceiver, 0,
                                             _endpoint, _adapter);
        }
        catch(const LocalException& ex)
        {
            try
            {
                transceiver->close();
            }
            catch(const Ice::LocalException&)
            {
                // Ignore.
            }

            if(_warn)
            {
                Warning out(_instance->initializationData().logger);
                out << "connection exception:\n" << ex << '\n' << _acceptor->toString();
            }
            return;
        }

        _connections.insert(connection);
    }

    assert(connection);

    auto self = shared_from_this();
    connection->start(
        [self](ConnectionIPtr conn)
        {
            self->connectionStartCompleted(std::move(conn));
        },
        [self](ConnectionIPtr conn, exception_ptr ex)
        {
            self->connectionStartFailed(std::move(conn), ex);
        });
}

void
IceInternal::IncomingConnectionFactory::finished(ThreadPoolCurrent&, bool close)
{
    unique_lock lock(_mutex);
    if(_state < StateClosed)
    {
        if(close)
        {
            closeAcceptor();
        }

        //
        // If the acceptor hasn't been explicitly stopped (which is the case if the acceptor got closed
        // because of an unexpected error), try to restart the acceptor in 1 second.
        //
        if(!_acceptorStopped)
        {
            _instance->timer()->schedule(make_shared<StartAcceptor>(shared_from_this(), _instance),
                                         IceUtil::Time::seconds(1));
        }
        return;
    }

    assert(_state >= StateClosed);
    setState(StateFinished);

    if(close)
    {
        closeAcceptor();
    }

#if TARGET_OS_IPHONE != 0
    lock.unlock();
    finish();
#endif
}

#if TARGET_OS_IPHONE != 0
void
IceInternal::IncomingConnectionFactory::finish()
{
    unregisterForBackgroundNotification(shared_from_this());
}
#endif

string
IceInternal::IncomingConnectionFactory::toString() const
{
    lock_guard lock(_mutex);
    if(_transceiver)
    {
        return _transceiver->toString();
    }
    else if(_acceptor)
    {
        return _acceptor->toString();
    }
    else
    {
        return string();
    }
}

NativeInfoPtr
IceInternal::IncomingConnectionFactory::getNativeInfo()
{
    if(_transceiver)
    {
        return _transceiver->getNativeInfo();
    }
    else if(_acceptor)
    {
        return _acceptor->getNativeInfo();
    }
    else
    {
        return 0;
    }
}

void
IceInternal::IncomingConnectionFactory::connectionStartCompleted(const Ice::ConnectionIPtr& connection)
{
    lock_guard lock(_mutex);

    //
    // Initialy, connections are in the holding state. If the factory is active
    // we activate the connection.
    //
    if(_state == StateActive)
    {
        connection->activate();
    }
}

void
IceInternal::IncomingConnectionFactory::connectionStartFailed(const Ice::ConnectionIPtr& /*connection*/,
                                                              exception_ptr)
{
    lock_guard lock(_mutex);
    if(_state >= StateClosed)
    {
        return;
    }

    //
    // Do not warn about connection exceptions here. The connection is not yet validated.
    //
}

//
// COMPILERFIX: The ConnectionFactory setup is broken out into a separate initialize
// function because when it was part of the constructor C++Builder 2007 apps would
// crash if an execption was thrown from any calls within the constructor.
//
IceInternal::IncomingConnectionFactory::IncomingConnectionFactory(const InstancePtr& instance,
                                                                  const EndpointIPtr& endpoint,
                                                                  const EndpointIPtr& publishedEndpoint,
                                                                  const shared_ptr<ObjectAdapterI>& adapter) :
    _instance(instance),
    _monitor(new FactoryACMMonitor(instance, dynamic_cast<ObjectAdapterI*>(adapter.get())->getACM())),
    _endpoint(endpoint),
    _publishedEndpoint(publishedEndpoint),
    _acceptorStarted(false),
    _acceptorStopped(false),
    _adapter(adapter),
    _warn(_instance->initializationData().properties->getPropertyAsInt("Ice.Warn.Connections") > 0),
    _state(StateHolding)
{
}

void
IceInternal::IncomingConnectionFactory::startAcceptor()
{
    lock_guard lock(_mutex);
    if(_state >= StateClosed || _acceptorStarted)
    {
        return;
    }

    _acceptorStopped = false;
    createAcceptor();
}

void
IceInternal::IncomingConnectionFactory::stopAcceptor()
{
    lock_guard lock(_mutex);
    if(_state >= StateClosed || !_acceptorStarted)
    {
        return;
    }

    _acceptorStopped = true;
    _acceptorStarted = false;
    if(_adapter->getThreadPool()->finish(shared_from_this(), true))
    {
        closeAcceptor();
    }
}

void
IceInternal::IncomingConnectionFactory::initialize()
{
    if(_instance->defaultsAndOverrides()->overrideTimeout)
    {
        _endpoint = _endpoint->timeout(_instance->defaultsAndOverrides()->overrideTimeoutValue);
    }

    if(_instance->defaultsAndOverrides()->overrideCompress)
    {
        _endpoint = _endpoint->compress(_instance->defaultsAndOverrides()->overrideCompressValue);
    }
    try
    {
        const_cast<TransceiverPtr&>(_transceiver) = _endpoint->transceiver();
        if(_transceiver)
        {
            if(_instance->traceLevels()->network >= 2)
            {
                Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
                out << "attempting to bind to " << _endpoint->protocol() << " socket\n" << _transceiver->toString();
            }
            const_cast<EndpointIPtr&>(_endpoint) = _transceiver->bind();
            ConnectionIPtr connection(ConnectionI::create(_adapter->getCommunicator(), _instance, 0, _transceiver, 0,
                                                          _endpoint, _adapter));
            connection->start(nullptr, nullptr);
            _connections.insert(connection);
        }
        else
        {
#if TARGET_OS_IPHONE != 0
            //
            // The notification center will call back on the factory to
            // start the acceptor if necessary.
            //
            registerForBackgroundNotification(shared_from_this());
#else
            createAcceptor();
#endif
        }
    }
    catch(const Ice::Exception&)
    {
        if(_transceiver)
        {
            try
            {
                _transceiver->close();
            }
            catch(const Ice::LocalException&)
            {
                // Ignore
            }
        }

        _state = StateFinished;
        _monitor->destroy();
        _connections.clear();
        throw;
    }
}

IceInternal::IncomingConnectionFactory::~IncomingConnectionFactory()
{
    assert(_state == StateFinished);
    assert(_connections.empty());
}

void
IceInternal::IncomingConnectionFactory::setState(State state)
{
    if(_state == state) // Don't switch twice.
    {
        return;
    }

    switch(state)
    {
        case StateActive:
        {
            if(_state != StateHolding) // Can only switch from holding to active.
            {
                return;
            }
            if(_acceptor)
            {
                if(_instance->traceLevels()->network >= 1)
                {
                    Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
                    out << "accepting " << _endpoint->protocol() << " connections at " << _acceptor->toString();
                }
                _adapter->getThreadPool()->_register(shared_from_this(), SocketOperationRead);
            }

            for(const auto& conn : _connections)
            {
                conn->activate();
            }
            break;
        }

        case StateHolding:
        {
            if(_state != StateActive) // Can only switch from active to holding.
            {
                return;
            }
            if(_acceptor)
            {
                if(_instance->traceLevels()->network >= 1)
                {
                    Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
                    out << "holding " << _endpoint->protocol() << " connections at " << _acceptor->toString();
                }
                _adapter->getThreadPool()->unregister(shared_from_this(), SocketOperationRead);
            }
            for(const auto& conn : _connections)
            {
                conn->hold();
            }
            break;
        }

        case StateClosed:
        {
            if(_acceptorStarted)
            {
                //
                // If possible, close the acceptor now to prevent new connections from
                // being accepted while we are deactivating. This is especially useful
                // if there are no more threads in the thread pool available to dispatch
                // the finish() call. Not all selector implementations do support this
                // however.
                //
                _acceptorStarted = false;
                if(_adapter->getThreadPool()->finish(shared_from_this(), true))
                {
                    closeAcceptor();
                }
            }
            else
            {
#if TARGET_OS_IPHONE != 0
                _adapter->getThreadPool()->dispatch(make_shared<FinishCall>(shared_from_this()));
#endif
                state = StateFinished;
            }

            for(const auto& conn : _connections)
            {
                conn->destroy(ConnectionI::ObjectAdapterDeactivated);
            }
            break;
        }

        case StateFinished:
        {
            assert(_state == StateClosed);
            break;
        }
    }

    _state = state;
    _conditionVariable.notify_all();
}

void
IceInternal::IncomingConnectionFactory::createAcceptor()
{
    try
    {
        assert(!_acceptorStarted);
        _acceptor = _endpoint->acceptor(_adapter->getName());
        assert(_acceptor);
        if(_instance->traceLevels()->network >= 2)
        {
            Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
            out << "attempting to bind to " << _endpoint->protocol() << " socket " << _acceptor->toString();
        }

        _endpoint = _acceptor->listen();
        if(_instance->traceLevels()->network >= 1)
        {
            Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
            out << "listening for " << _endpoint->protocol() << " connections\n" << _acceptor->toDetailedString();
        }

        _adapter->getThreadPool()->initialize(shared_from_this());
        if(_state == StateActive)
        {
            _adapter->getThreadPool()->_register(shared_from_this(), SocketOperationRead);
        }

        _acceptorStarted = true;
    }
    catch(const Ice::Exception&)
    {
        if(_acceptor)
        {
            _acceptor->close();
        }
        throw;
    }
}

void
IceInternal::IncomingConnectionFactory::closeAcceptor()
{
    assert(_acceptor);

    if(_instance->traceLevels()->network >= 1)
    {
        Trace out(_instance->initializationData().logger, _instance->traceLevels()->networkCat);
        out << "stopping to accept " << _endpoint->protocol() << " connections at " << _acceptor->toString();
    }

    assert(!_acceptorStarted);
    _acceptor->close();
}
