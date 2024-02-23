//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Glacier2/SessionHelper.h>

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>

#include <algorithm> // required by max

using namespace std;

namespace Glacier2
{

class SessionThreadCallback : public std::enable_shared_from_this<SessionThreadCallback>
{
public:

    SessionThreadCallback(const Glacier2::SessionFactoryHelperPtr& factory) :
        _factory(factory)
    {
    }

    void add(const SessionHelper* session, thread&& thread)
    {
        _factory->addThread(session, std::move(thread));
    }

    thread pop(const SessionHelper* session)
    {
        return _factory->popThread(session);
    }

private:

    const SessionFactoryHelperPtr _factory;
};
using SessionThreadCallbackPtr = std::shared_ptr<SessionThreadCallback>;

};

namespace
{

class ConnectStrategy
{

public:

    virtual Glacier2::SessionPrxPtr connect(const Glacier2::RouterPrxPtr& router) = 0;
};
using ConnectStrategyPtr = std::shared_ptr<ConnectStrategy>;

class Disconnected : public Ice::DispatcherCall
{

public:

    Disconnected(const Glacier2::SessionHelperPtr& session, const Glacier2::SessionCallbackPtr& callback) :
        _session(session),
        _callback(callback)
    {
    }

    virtual
    void run()
    {
        _callback->disconnected(_session);
    }

private:

    const Glacier2::SessionHelperPtr _session;
    const Glacier2::SessionCallbackPtr _callback;
};

class SessionHelperI : public Glacier2::SessionHelper, public std::enable_shared_from_this<SessionHelperI>
{

public:

    SessionHelperI(const Glacier2::SessionThreadCallbackPtr&, const Glacier2::SessionCallbackPtr&,
                   const Ice::InitializationData&, const string&, bool);
    void destroy();
    Ice::CommunicatorPtr communicator() const;
    std::string categoryForClient() const;
    Ice::ObjectPrx addWithUUID(const Ice::ObjectPtr&);
    Glacier2::SessionPrxPtr session() const;
    bool isConnected() const;
    Ice::ObjectAdapterPtr objectAdapter();

    friend class Glacier2::SessionFactoryHelper;

private:

    Ice::ObjectAdapterPtr internalObjectAdapter();
    void connected(const Glacier2::RouterPrxPtr&, const Glacier2::SessionPrxPtr&);
    void destroyInternal(const Ice::DispatcherCallPtr&);
    void destroyCommunicator();
    void connectFailed();

    void connect(const std::map<std::string, std::string>&);
    void connect(const std::string&, const std::string&, const std::map<std::string, std::string>&);

    void connectImpl(const ConnectStrategyPtr&);
    void dispatchCallback(const Ice::DispatcherCallPtr&, const Ice::ConnectionPtr&);
    void dispatchCallbackAndWait(const Ice::DispatcherCallPtr&, const Ice::ConnectionPtr&);

    mutable std::mutex _mutex;
    Ice::CommunicatorPtr _communicator;
    Ice::ObjectAdapterPtr _adapter;
    Glacier2::RouterPrxPtr _router;
    Glacier2::SessionPrxPtr _session;
    std::string _category;
    bool _connected;
    bool _destroy;
    const Ice::InitializationData _initData;
    Glacier2::SessionThreadCallbackPtr _threadCB;
    const Glacier2::SessionCallbackPtr _callback;
    const string _finder;
    const bool _useCallbacks;
};
using SessionHelperIPtr = std::shared_ptr<SessionHelperI>;

}

SessionHelperI::SessionHelperI(const Glacier2::SessionThreadCallbackPtr& threadCB,
                               const Glacier2::SessionCallbackPtr& callback,
                               const Ice::InitializationData& initData,
                               const string& finderStr,
                               bool useCallbacks) :
    _connected(false),
    _destroy(false),
    _initData(initData),
    _threadCB(threadCB),
    _callback(callback),
    _finder(finderStr),
    _useCallbacks(useCallbacks)
{
}

void
SessionHelperI::destroy()
{
    lock_guard lock(_mutex);
    if(_destroy)
    {
        return;
    }
    _destroy = true;

    // Run destroy in a thread because it can block.
    if (!_connected)
    {
        // In this case a connecting session is being destroyed. We destroy the communicator to trigger the immediate
        // failure of the connection establishment.

        auto destroyThread = std::thread(
            [session = shared_from_this(), previous = _threadCB->pop(this)]() mutable
            {
                session->destroyCommunicator();

                // Join the connect thread to free resources.
                if (previous.joinable())
                {
                    previous.join();
                }
            });
        _threadCB->add(this, std::move(destroyThread));
    }
    else
    {
        auto destroyThread = std::thread(
            [session = shared_from_this(), previous = _threadCB->pop(this)]() mutable
            {
                session->destroyInternal(make_shared<Disconnected>(session, session->_callback));

                // Join the connect thread to free resources.
                if (previous.joinable())
                {
                    previous.join();
                }
            });
        _threadCB->add(this, std::move(destroyThread));
    }
    _threadCB = nullptr;
}

Ice::CommunicatorPtr
SessionHelperI::communicator() const
{
    lock_guard lock(_mutex);
    return _communicator;
}

string
SessionHelperI::categoryForClient() const
{
    lock_guard lock(_mutex);
    if(!_router)
    {
        throw Glacier2::SessionNotExistException();
    }
    return _category;
}

Ice::ObjectPrx
SessionHelperI::addWithUUID(const Ice::ObjectPtr& servant)
{
    lock_guard lock(_mutex);
    if(!_router)
    {
        throw Glacier2::SessionNotExistException();
    }
    Ice::Identity id;
    id.name = Ice::generateUUID();
    id.category = _category;
    return internalObjectAdapter()->add(servant, id);
}

Glacier2::SessionPrxPtr
SessionHelperI::session() const
{
    lock_guard lock(_mutex);
    return _session;
}

bool
SessionHelperI::isConnected() const
{
    lock_guard lock(_mutex);
    return _connected;
}

Ice::ObjectAdapterPtr
SessionHelperI::objectAdapter()
{
    lock_guard lock(_mutex);
    return internalObjectAdapter();
}

Glacier2::SessionHelper::~SessionHelper()
{
    // Out of line to avoid weak vtable
}

Ice::ObjectAdapterPtr
SessionHelperI::internalObjectAdapter()
{
    if(!_router)
    {
        throw Glacier2::SessionNotExistException();
    }
    if(!_useCallbacks)
    {
        throw Ice::InitializationException(__FILE__, __LINE__,
            "Object adapter not available, call SessionFactoryHelper.setUseCallbacks(true)");
    }
    return _adapter;
}

Glacier2::SessionCallback::~SessionCallback()
{
    // Out of line to avoid weak vtable
}

namespace
{

class ConnectStrategySecureConnection final : public ConnectStrategy
{

public:

    ConnectStrategySecureConnection(const map<string, string>& context) :
        _context(context)
    {
    }

    virtual Glacier2::SessionPrxPtr
    connect(const Glacier2::RouterPrxPtr& router)
    {
        return router->createSessionFromSecureConnection(_context);
    }

private:

    const map<string, string> _context;
};

class ConnectStrategyUserPassword final : public ConnectStrategy
{

public:

    ConnectStrategyUserPassword(const string& user, const string& password, const map<string, string>& context) :
        _user(user),
        _password(password),
        _context(context)
    {
    }

    virtual Glacier2::SessionPrxPtr
    connect(const Glacier2::RouterPrxPtr& router)
    {
        return router->createSession(_user, _password, _context);
    }

private:

    const string _user;
    const string _password;
    const map<string, string> _context;
};

}

void
SessionHelperI::connect(const map<string, string>& context)
{
    lock_guard lock(_mutex);
    connectImpl(make_shared<ConnectStrategySecureConnection>(context));
}

void
SessionHelperI::connect(const string& user, const string& password, const map<string, string>& context)
{
    lock_guard lock(_mutex);
    connectImpl(make_shared<ConnectStrategyUserPassword>(user, password, context));
}

void
SessionHelperI::destroyInternal(const Ice::DispatcherCallPtr& disconnected)
{
    assert(_destroy);
    Ice::CommunicatorPtr communicator;
    Glacier2::RouterPrxPtr router;
    {
        lock_guard lock(_mutex);
        router = _router;
        _router = nullopt;
        _connected = false;

        communicator = _communicator;
    }

    if(router)
    {
        try
        {
            router->destroySession();
        }
        catch(const Ice::ConnectionLostException&)
        {
            //
            // Expected if another thread invoked on an object from the session concurrently.
            //
        }
        catch(const Glacier2::SessionNotExistException&)
        {
            //
            // This can also occur.
            //
        }
        catch(const std::exception& ex)
        {
            //
            // Not expected.
            //
            if(communicator)
            {
                Ice::Warning warn(communicator->getLogger());
                warn << "SessionHelper: unexpected exception when destroying the session:\n" << ex;
            }
        }
    }

    if(communicator)
    {
        communicator->destroy();
    }
    dispatchCallback(disconnected, nullptr);
}

void
SessionHelperI::destroyCommunicator()
{
    Ice::CommunicatorPtr communicator;
    {
        lock_guard lock(_mutex);
        communicator = _communicator;
    }

    if(communicator)
    {
        communicator->destroy();
    }
}

void
SessionHelperI::connectFailed()
{
    Ice::CommunicatorPtr communicator;
    {
        lock_guard lock(_mutex);
        communicator = _communicator;
    }

    if(communicator)
    {
        communicator->destroy();
    }
}

namespace
{

class ConnectFailed : public Ice::DispatcherCall
{

public:

    ConnectFailed(const Glacier2::SessionCallbackPtr& callback, const Glacier2::SessionHelperPtr& session,
                  std::exception_ptr ex) :
        _callback(callback),
        _session(session)
    {
        _ex = ex;
    }

    virtual void
    run()
    {
        _callback->connectFailed(_session, _ex);
    }

private:

    const Glacier2::SessionCallbackPtr _callback;
    const Glacier2::SessionHelperPtr _session;
    std::exception_ptr _ex;
};

class CreatedCommunicator : public Ice::DispatcherCall
{

public:

    CreatedCommunicator(const Glacier2::SessionCallbackPtr& callback, const Glacier2::SessionHelperPtr& session) :
        _callback(callback),
        _session(session)
    {
    }

    virtual void
    run()
    {
        _callback->createdCommunicator(_session);
    }

private:

    const Glacier2::SessionCallbackPtr _callback;
    const Glacier2::SessionHelperPtr _session;
};

}

void
SessionHelperI::connectImpl(const ConnectStrategyPtr& factory)
{
    promise<void> startPromise;

    assert(!_destroy);
    auto thread = std::thread(
        [session = shared_from_this(),
         callback = _callback,
         factory = std::move(factory),
         startFuture = startPromise.get_future()]()
        {
            startFuture.wait(); // Wait for the thread to be registered with the thread callback.

            Ice::CommunicatorPtr communicator;
            try
            {
                lock_guard lock(session->_mutex);
                communicator = Ice::initialize(session->_initData);
                session->_communicator = communicator;
            }
            catch(const Ice::LocalException&)
            {
                {
                    lock_guard lock(session->_mutex);
                    session->_destroy = true;
                }
                session->dispatchCallback(make_shared<ConnectFailed>(callback, session, current_exception()), nullptr);
                return;
            }

            try
            {
                if(!communicator->getDefaultRouter())
                {
                    Ice::RouterFinderPrxPtr finder;
                    try
                    {
                        finder = Ice::uncheckedCast<Ice::RouterFinderPrx>(communicator->stringToProxy(session->_finder));
                        communicator->setDefaultRouter(finder->getRouter());
                    }
                    catch(const Ice::CommunicatorDestroyedException&)
                    {
                        session->dispatchCallback(make_shared<ConnectFailed>(callback, session, current_exception()), 0);
                        return;
                    }
                    catch(const Ice::Exception&)
                    {
                        //
                        // In case of error getting router identity from RouterFinder use
                        // default identity.
                        //
                        Ice::Identity ident;
                        ident.category = "Glacier2";
                        ident.name = "router";
                        communicator->setDefaultRouter(Ice::uncheckedCast<Ice::RouterPrx>(finder->ice_identity(ident)));
                    }
                }
                session->dispatchCallbackAndWait(make_shared<CreatedCommunicator>(callback, session), nullptr);
                auto routerPrx = Ice::uncheckedCast<Glacier2::RouterPrx>(communicator->getDefaultRouter());
                Glacier2::SessionPrxPtr sessionPrx = factory->connect(routerPrx);
                session->connected(routerPrx, sessionPrx);
            }
            catch(const Ice::Exception&)
            {
                try
                {
                    session->connectFailed();
                }
                catch(...)
                {
                }

                session->dispatchCallback(make_shared<ConnectFailed>(session->_callback, session, current_exception()), nullptr);
            }
        });
    _threadCB->add(this, std::move(thread));
    startPromise.set_value();
}

namespace
{

class Connected : public Ice::DispatcherCall
{

public:

    Connected(const Glacier2::SessionCallbackPtr& callback, const Glacier2::SessionHelperPtr& session) :
        _callback(callback),
        _session(session)
    {
    }

    virtual void
    run()
    {
        try
        {
            _callback->connected(_session);
        }
        catch(const Glacier2::SessionNotExistException&)
        {
            _session->destroy();
        }
    }

private:

    const Glacier2::SessionCallbackPtr _callback;
    const Glacier2::SessionHelperPtr _session;
};

}

void
SessionHelperI::connected(const Glacier2::RouterPrxPtr& router, const Glacier2::SessionPrxPtr& session)
{
    //
    // Remote invocation should be done without acquiring a mutex lock.
    //
    assert(router);
    Ice::ConnectionPtr conn = router->ice_getCachedConnection();
    string category = router->getCategoryForClient();
    int32_t acmTimeout = 0;
    try
    {
        acmTimeout = router->getACMTimeout();
    }
    catch(const Ice::OperationNotExistException&)
    {
    }

    if(acmTimeout <= 0)
    {
        acmTimeout = static_cast<int32_t>(router->getSessionTimeout());
    }

    //
    // We create the callback object adapter here because createObjectAdapter internally
    // makes synchronous RPCs to the router. We can't create the OA on-demand when the
    // client calls objectAdapter() or addWithUUID() because they can be called from the
    // GUI thread.
    //
    if(_useCallbacks)
    {
        _adapter = _communicator->createObjectAdapterWithRouter("", router.value());
        _adapter->activate();
    }

    bool destroy;
    {
        lock_guard lock(_mutex);
        _router = router;
        destroy = _destroy;

        if(!_destroy)
        {
            //
            // Cache the category.
            //
            _category = category;

            //
            // Assign the session after _destroy is checked.
            //
            _session = session;
            _connected = true;

            if(acmTimeout > 0)
            {
                Ice::ConnectionPtr connection = _router->ice_getCachedConnection();
                assert(connection);
                connection->setACM(acmTimeout, nullopt, Ice::ACMHeartbeat::HeartbeatAlways);
                auto self = shared_from_this();
                connection->setCloseCallback([self](Ice::ConnectionPtr)
                {
                    self->destroy();
                });
            }
        }
    }

    if(destroy)
    {
        //
        // connected() is only called from the ConnectThread so it is ok to
        // call destroyInternal here.
        //
        destroyInternal(make_shared<Disconnected>(shared_from_this(), _callback));
    }
    else
    {
        dispatchCallback(make_shared<Connected>(_callback, shared_from_this()), conn);
    }
}

void
SessionHelperI::dispatchCallback(const Ice::DispatcherCallPtr& call, const Ice::ConnectionPtr& conn)
{
    if(_initData.dispatcher)
    {
        _initData.dispatcher([call]()
            {
                call->run();
            },
            conn);
    }
    else
    {
        call->run();
    }
}

void
SessionHelperI::dispatchCallbackAndWait(const Ice::DispatcherCallPtr& call, const Ice::ConnectionPtr& conn)
{
    if(_initData.dispatcher)
    {
        condition_variable condition;
        mutex mutex;

        _initData.dispatcher([&condition, call = std::move(call)]()
            {
                call->run();
                condition.notify_one();
            },
            conn);
        unique_lock lock(mutex);
        condition.wait(lock);
    }
    else
    {
        call->run();
    }
}

Glacier2::SessionFactoryHelper::SessionFactoryHelper(const SessionCallbackPtr& callback) :
    _routerHost("localhost"),
    _protocol("ssl"),
    _port(0),
    _timeout(10000),
    _callback(callback),
    _useCallbacks(true)
{
    _initData.properties = Ice::createProperties();
    setDefaultProperties();
}

Glacier2::SessionFactoryHelper::SessionFactoryHelper(const Ice::InitializationData& initData,
                                                     const SessionCallbackPtr& callback) :
    _routerHost("localhost"),
    _protocol("ssl"),
    _port(0),
    _timeout(10000),
    _initData(initData),
    _callback(callback),
    _useCallbacks(true)
{
    if(!initData.properties)
    {
        _initData.properties = Ice::createProperties();
    }
    setDefaultProperties();
}

Glacier2::SessionFactoryHelper::SessionFactoryHelper(const Ice::PropertiesPtr& properties,
                                                     const SessionCallbackPtr& callback) :
    _routerHost("localhost"),
    _protocol("ssl"),
    _port(0),
    _timeout(10000),
    _callback(callback),
    _useCallbacks(true)
{
    if(!properties)
    {
        throw Ice::InitializationException(
            __FILE__, __LINE__, "Attempt to create a SessionFactoryHelper with a null Properties argument");
    }
    _initData.properties = properties;
    setDefaultProperties();
}

Glacier2::SessionFactoryHelper::~SessionFactoryHelper()
{
    lock_guard lock(_mutex);
    if(!_threads.empty() && Ice::getProcessLogger())
    {
        Ice::Warning warn(Ice::getProcessLogger());
        warn << "Glacier2::SessionFactoryHelper::destroy() has not been called, threads won't be joined";
    }
}

void
Glacier2::SessionFactoryHelper::addThread(const SessionHelper* session, thread&& newThread)
{
    // A SessionHelper can only ever have one thread running. Therefore any currently registered thread for the same
    // session must be finished, before calling this method.
    lock_guard lock(_mutex);
    assert(_threads.find(session) == _threads.end());
    _threads.emplace(make_pair(session, std::move(newThread)));
}

thread
Glacier2::SessionFactoryHelper::popThread(const SessionHelper* session)
{
    // A SessionHelper can only ever have one thread running. Therefore any currently registered thread for the same
    // session must be finished, before calling this method.
    lock_guard lock(_mutex);
    auto p = _threads.find(session);
    if (p == _threads.end())
    {
        return thread();
    }
    else
    {
        auto thread = std::move(p->second);
        _threads.erase(p);
        return thread;
    }
}

void
Glacier2::SessionFactoryHelper::destroy()
{
    lock_guard lock(_mutex);
    for(auto& p : _threads)
    {
        if (p.second.joinable())
        {
            p.second.join();
        }
    }
    _threads.clear();
}

void
Glacier2::SessionFactoryHelper::setRouterIdentity(const Ice::Identity& identity)
{
    lock_guard lock(_mutex);
    _identity = identity;
}

Ice::Identity
Glacier2::SessionFactoryHelper::getRouterIdentity() const
{
    lock_guard lock(_mutex);
    return _identity;
}

void
Glacier2::SessionFactoryHelper::setRouterHost(const string& hostname)
{
    lock_guard lock(_mutex);
    _routerHost = hostname;
}

string
Glacier2::SessionFactoryHelper::getRouterHost() const
{
    lock_guard lock(_mutex);
    return _routerHost;
}

void
Glacier2::SessionFactoryHelper::setSecure(bool secure)
{
    setProtocol(secure ? "ssl" : "tcp");
}

bool
Glacier2::SessionFactoryHelper::getSecure() const
{
    return getProtocol() == "ssl";
}

void
Glacier2::SessionFactoryHelper::setProtocol(const string& protocol)
{
    lock_guard lock(_mutex);
    if(protocol != "tcp" &&
       protocol != "ssl" &&
       protocol != "ws" &&
       protocol != "wss")
    {
        throw invalid_argument("Unknown protocol `" + protocol + "'");
    }
    _protocol = protocol;
}

string
Glacier2::SessionFactoryHelper::getProtocol() const
{
    lock_guard lock(_mutex);
    return _protocol;
}

void
Glacier2::SessionFactoryHelper::setTimeout(int timeout)
{
    lock_guard lock(_mutex);
    _timeout = timeout;
}

int
Glacier2::SessionFactoryHelper::getTimeout() const
{
    lock_guard lock(_mutex);
    return _timeout;
}

void
Glacier2::SessionFactoryHelper::setPort(int port)
{
    lock_guard lock(_mutex);
    _port = port;
}

int
Glacier2::SessionFactoryHelper::getPort() const
{
    lock_guard lock(_mutex);
    return getPortInternal();
}

int
Glacier2::SessionFactoryHelper::getPortInternal() const
{
    //  Must be called with the muext lock
    return _port == 0 ? ((_protocol == "ssl" || _protocol == "wss") ? GLACIER2_SSL_PORT : GLACIER2_TCP_PORT) : _port;
}

Ice::InitializationData
Glacier2::SessionFactoryHelper::getInitializationData() const
{
    lock_guard lock(_mutex);
    return _initData;
}

void
Glacier2::SessionFactoryHelper::setConnectContext(const map<string, string>& context)
{
    lock_guard lock(_mutex);
    _context = context;
}

void
Glacier2::SessionFactoryHelper::setUseCallbacks(bool useCallbacks)
{
    lock_guard lock(_mutex);
    _useCallbacks = useCallbacks;
}

bool
Glacier2::SessionFactoryHelper::getUseCallbacks() const
{
    lock_guard lock(_mutex);
    return _useCallbacks;
}

Glacier2::SessionHelperPtr
Glacier2::SessionFactoryHelper::connect()
{
    SessionHelperIPtr session;
    map<string, string> context;
    {
        lock_guard lock(_mutex);
        session = make_shared<SessionHelperI>(make_shared<SessionThreadCallback>(shared_from_this()),
                                              _callback,
                                              createInitData(),
                                              getRouterFinderStr(),
                                              _useCallbacks);
        context = _context;
    }
    session->connect(context);
    return session;
}

Glacier2::SessionHelperPtr
Glacier2::SessionFactoryHelper::connect(const string& user,  const string& password)
{
    SessionHelperIPtr session;
    map<string, string> context;
    {
        lock_guard lock(_mutex);
        session = make_shared<SessionHelperI>(make_shared<SessionThreadCallback>(shared_from_this()),
                                              _callback,
                                              createInitData(),
                                              getRouterFinderStr(),
                                              _useCallbacks);
        context = _context;
    }
    session->connect(user, password, _context);
    return session;
}

Ice::InitializationData
Glacier2::SessionFactoryHelper::createInitData()
{
    //
    // Clone the initialization data and properties.
    //
    Ice::InitializationData initData = _initData;
    initData.properties = initData.properties->clone();

    if(initData.properties->getProperty("Ice.Default.Router").size() == 0 && !_identity.name.empty())
    {
        initData.properties->setProperty("Ice.Default.Router", createProxyStr(_identity));
    }

    //
    // If using a secure connection setup the IceSSL plug-in, if IceSSL
    // plug-in has already been setup we don't want to override the
    // configuration so it can be loaded from a custom location.
    //
    if((_protocol == "ssl" || _protocol == "wss") &&
       initData.properties->getProperty("Ice.Plugin.IceSSL").empty())
    {
        initData.properties->setProperty("Ice.Plugin.IceSSL","IceSSL:createIceSSL");
    }

    return initData;
}

string
Glacier2::SessionFactoryHelper::getRouterFinderStr()
{
    Ice::Identity ident;
    ident.category = "Ice";
    ident.name = "RouterFinder";

    return createProxyStr(ident);
}

string
Glacier2::SessionFactoryHelper::createProxyStr(const Ice::Identity& ident)
{
    ostringstream os;
    os << "\"" << identityToString(ident, Ice::ToStringMode::Unicode) << "\":" << _protocol
       << " -p " << getPortInternal() << " -h \"" << _routerHost << "\"";

    if(_timeout > 0)
    {
        os << " -t " << _timeout;
    }
    return os.str();
}

void
Glacier2::SessionFactoryHelper::setDefaultProperties()
{
    assert(_initData.properties);
    _initData.properties->setProperty("Ice.RetryIntervals", "-1");
}
