//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_CONNECTION_FACTORY_H
#define ICE_CONNECTION_FACTORY_H

#include <Ice/CommunicatorF.h>
#include <Ice/ConnectionFactoryF.h>
#include <Ice/ConnectionI.h>
#include <Ice/InstanceF.h>
#include <Ice/ObjectAdapterF.h>
#include <Ice/EndpointIF.h>
#include <Ice/Endpoint.h>
#include <Ice/ConnectorF.h>
#include <Ice/AcceptorF.h>
#include <Ice/TransceiverF.h>
#include <Ice/RouterInfoF.h>
#include <Ice/EventHandler.h>
#include <Ice/EndpointI.h>
#include <Ice/InstrumentationF.h>
#include <Ice/ACMF.h>
#include <Ice/Comparable.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <set>

namespace Ice
{

class ObjectAdapterI;

}

namespace IceInternal
{

template<typename T> class ThreadPoolMessage;

class OutgoingConnectionFactory final : public std::enable_shared_from_this<OutgoingConnectionFactory>
{
public:

    class CreateConnectionCallback
    {
    public:

        virtual void setConnection(const Ice::ConnectionIPtr&, bool) = 0;
        virtual void setException(std::exception_ptr) = 0;
    };
    using CreateConnectionCallbackPtr = std::shared_ptr<CreateConnectionCallback>;

    void destroy();

    void updateConnectionObservers();

    void waitUntilFinished();

    void create(const std::vector<EndpointIPtr>&, bool, Ice::EndpointSelectionType, const CreateConnectionCallbackPtr&);
    void setRouterInfo(const RouterInfoPtr&);
    void removeAdapter(const Ice::ObjectAdapterPtr&);
    void flushAsyncBatchRequests(const CommunicatorFlushBatchAsyncPtr&, Ice::CompressBatch);

    OutgoingConnectionFactory(const Ice::CommunicatorPtr&, const InstancePtr&);
    ~OutgoingConnectionFactory();
    friend class Instance;

private:

    struct ConnectorInfo
    {
        ConnectorInfo(const ConnectorPtr& c, const EndpointIPtr& e) : connector(c), endpoint(e)
        {
        }

        bool operator==(const ConnectorInfo& other) const;

        ConnectorPtr connector;
        EndpointIPtr endpoint;
    };

    class ConnectCallback final : public std::enable_shared_from_this<ConnectCallback>
    {
    public:

        ConnectCallback(
            const InstancePtr&,
            const OutgoingConnectionFactoryPtr&,
            const std::vector<EndpointIPtr>&, bool,
            const CreateConnectionCallbackPtr&,
            Ice::EndpointSelectionType);

        virtual void connectionStartCompleted(const Ice::ConnectionIPtr&);
        virtual void connectionStartFailed(const Ice::ConnectionIPtr&, std::exception_ptr);

        virtual void connectors(const std::vector<ConnectorPtr>&);
        virtual void exception(std::exception_ptr);

        void getConnectors();
        void nextEndpoint();

        void getConnection();
        void nextConnector();

        void setConnection(const Ice::ConnectionIPtr&, bool);
        void setException(std::exception_ptr);

        bool hasConnector(const ConnectorInfo&);
        bool removeConnectors(const std::vector<ConnectorInfo>&);
        void removeFromPending();

        bool operator<(const ConnectCallback&) const;

    private:

        bool connectionStartFailedImpl(std::exception_ptr);

        const InstancePtr _instance;
        const OutgoingConnectionFactoryPtr _factory;
        const std::vector<EndpointIPtr> _endpoints;
        const bool _hasMore;
        const CreateConnectionCallbackPtr _callback;
        const Ice::EndpointSelectionType _selType;
        Ice::Instrumentation::ObserverPtr _observer;
        std::vector<EndpointIPtr>::const_iterator _endpointsIter;
        std::vector<ConnectorInfo> _connectors;
        std::vector<ConnectorInfo>::const_iterator _iter;
    };
    using ConnectCallbackPtr = std::shared_ptr<ConnectCallback>;
    friend class ConnectCallback;

    std::vector<EndpointIPtr> applyOverrides(const std::vector<EndpointIPtr>&);
    Ice::ConnectionIPtr findConnection(const std::vector<EndpointIPtr>&, bool&);
    void incPendingConnectCount();
    void decPendingConnectCount();
    Ice::ConnectionIPtr getConnection(const std::vector<ConnectorInfo>&, const ConnectCallbackPtr&, bool&);
    void finishGetConnection(const std::vector<ConnectorInfo>&, const ConnectorInfo&, const Ice::ConnectionIPtr&,
                             const ConnectCallbackPtr&);
    void finishGetConnection(const std::vector<ConnectorInfo>&, std::exception_ptr, const ConnectCallbackPtr&);

    bool addToPending(const ConnectCallbackPtr&, const std::vector<ConnectorInfo>&);
    void removeFromPending(const ConnectCallbackPtr&, const std::vector<ConnectorInfo>&);

    Ice::ConnectionIPtr findConnection(const std::vector<ConnectorInfo>&, bool&);
    Ice::ConnectionIPtr createConnection(const TransceiverPtr&, const ConnectorInfo&);

    void handleException(std::exception_ptr, bool);
    void handleConnectionException(std::exception_ptr, bool);

    Ice::CommunicatorPtr _communicator;
    const InstancePtr _instance;
    const FactoryACMMonitorPtr _monitor;
    bool _destroyed;

    using ConnectCallbackSet = std::set<ConnectCallbackPtr, Ice::TargetCompare<ConnectCallbackPtr, std::less>>;

    std::multimap<ConnectorPtr, Ice::ConnectionIPtr, Ice::TargetCompare<ConnectorPtr, std::less>> _connections;
    std::map<ConnectorPtr, ConnectCallbackSet, Ice::TargetCompare<ConnectorPtr, std::less>> _pending;

    std::multimap<EndpointIPtr, Ice::ConnectionIPtr, Ice::TargetCompare<EndpointIPtr, std::less>> _connectionsByEndpoint;
    int _pendingConnectCount;
    std::mutex _mutex;
    std::condition_variable _conditionVariable;
};

class IncomingConnectionFactory final : public EventHandler
{
public:

    IncomingConnectionFactory(const InstancePtr&, const EndpointIPtr&, const EndpointIPtr&,
                              const std::shared_ptr<Ice::ObjectAdapterI>&);
    void activate();
    void hold();
    void destroy();

    void startAcceptor();
    void stopAcceptor();

    void updateConnectionObservers();

    void waitUntilHolding() const;
    void waitUntilFinished();

    bool isLocal(const EndpointIPtr&) const;
    EndpointIPtr endpoint() const;
    std::list<Ice::ConnectionIPtr> connections() const;
    void flushAsyncBatchRequests(const CommunicatorFlushBatchAsyncPtr&, Ice::CompressBatch);

    //
    // Operations from EventHandler
    //

#if defined(ICE_USE_IOCP)
    virtual bool startAsync(SocketOperation);
    virtual bool finishAsync(SocketOperation);
#endif

    virtual void message(ThreadPoolCurrent&);
    virtual void finished(ThreadPoolCurrent&, bool);
#if TARGET_OS_IPHONE != 0
    void finish();
#endif
    virtual std::string toString() const;
    virtual NativeInfoPtr getNativeInfo();

    virtual void connectionStartCompleted(const Ice::ConnectionIPtr&);
    virtual void connectionStartFailed(const Ice::ConnectionIPtr&, std::exception_ptr);
    void initialize();
    ~IncomingConnectionFactory();

    std::shared_ptr<IncomingConnectionFactory> shared_from_this()
    {
        return std::static_pointer_cast<IncomingConnectionFactory>(EventHandler::shared_from_this());
    }

private:

    friend class Ice::ObjectAdapterI;

    enum State
    {
        StateActive,
        StateHolding,
        StateClosed,
        StateFinished
    };

    void setState(State);

    void createAcceptor();
    void closeAcceptor();

    const InstancePtr _instance;
    const FactoryACMMonitorPtr _monitor;

    AcceptorPtr _acceptor;
    const TransceiverPtr _transceiver;
    EndpointIPtr _endpoint;
    EndpointIPtr _publishedEndpoint;

    bool _acceptorStarted;
    bool _acceptorStopped;

    std::shared_ptr<Ice::ObjectAdapterI> _adapter;
    const bool _warn;
    std::set<Ice::ConnectionIPtr> _connections;
    State _state;

#if defined(ICE_USE_IOCP)
    std::exception_ptr _acceptorException;
#endif
    mutable std::mutex _mutex;
    mutable std::condition_variable _conditionVariable;

    // For locking the _mutex
    template<typename T> friend class IceInternal::ThreadPoolMessage;
};

}

#endif
