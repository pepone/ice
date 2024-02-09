//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_IP_ENDPOINT_I_H
#define ICE_IP_ENDPOINT_I_H

#include <IceUtil/Config.h>
#include <IceUtil/Thread.h>
#include <IceUtil/Monitor.h>
#include <Ice/IPEndpointIF.h>
#include <Ice/EndpointI.h>
#include <Ice/Network.h>
#include <Ice/ProtocolInstanceF.h>
#include <Ice/ObserverHelper.h>

#include <deque>
#include <mutex>

namespace IceInternal
{

class ICE_API IPEndpointInfoI : public Ice::IPEndpointInfo
{
public:

    IPEndpointInfoI(const EndpointIPtr&);
    virtual ~IPEndpointInfoI();

    virtual Ice::Short type() const noexcept;
    virtual bool datagram() const noexcept;
    virtual bool secure() const noexcept;

private:

    const EndpointIPtr _endpoint;
};

class ICE_API IPEndpointI : public EndpointI, public std::enable_shared_from_this<IPEndpointI>
{
public:

    virtual void streamWriteImpl(Ice::OutputStream*) const;

    virtual Ice::EndpointInfoPtr getInfo() const noexcept;
    virtual Ice::Short type() const;
    virtual const std::string& protocol() const;
    virtual bool secure() const;

    virtual const std::string& connectionId() const;
    virtual EndpointIPtr connectionId(const ::std::string&) const;

    virtual void connectorsAsync(
        Ice::EndpointSelectionType,
        std::function<void(const std::vector<ConnectorPtr>&)>,
        std::function<void(std::exception_ptr)>) const;
    virtual std::vector<EndpointIPtr> expandIfWildcard() const;
    virtual std::vector<EndpointIPtr> expandHost(EndpointIPtr&) const;
    virtual bool equivalent(const EndpointIPtr&) const;
    virtual ::Ice::Int hash() const;
    virtual std::string options() const;

    virtual bool operator==(const Ice::Endpoint&) const;
    virtual bool operator<(const Ice::Endpoint&) const;

    virtual std::vector<ConnectorPtr> connectors(const std::vector<Address>&, const NetworkProxyPtr&) const;

    virtual void hashInit(Ice::Int&) const;
    virtual void fillEndpointInfo(Ice::IPEndpointInfo*) const;

    using EndpointI::connectionId;

    virtual void initWithOptions(std::vector<std::string>&, bool);

protected:

    friend class EndpointHostResolver;

    virtual bool checkOption(const std::string&, const std::string&, const std::string&);

    virtual ConnectorPtr createConnector(const Address& address, const NetworkProxyPtr&) const = 0;
    virtual IPEndpointIPtr createEndpoint(const std::string&, int, const std::string&) const = 0;

    IPEndpointI(const ProtocolInstancePtr&, const std::string&, int, const Address&, const std::string&);
    IPEndpointI(const ProtocolInstancePtr&);
    IPEndpointI(const ProtocolInstancePtr&, Ice::InputStream*);

    const ProtocolInstancePtr _instance;
    const std::string _host;
    const int _port;
    const Address _sourceAddr;
    const std::string _connectionId;

private:

    mutable bool _hashInitialized;
    mutable Ice::Int _hashValue;
    mutable std::mutex _hashMutex;
};

class ICE_API EndpointHostResolver : public IceUtil::Thread
{
public:

    EndpointHostResolver(const InstancePtr&);

    void resolve(
        const std::string&,
        int,
        Ice::EndpointSelectionType,
        const IPEndpointIPtr&,
        std::function<void(const std::vector<ConnectorPtr>&)>,
        std::function<void(std::exception_ptr)>);
    void destroy();

    virtual void run();
    void updateObserver();

private:

    struct ResolveEntry
    {
        std::string host;
        int port;
        Ice::EndpointSelectionType selType;
        IPEndpointIPtr endpoint;
        std::function<void(const std::vector<ConnectorPtr>&)> response;
        std::function<void(std::exception_ptr)> exception;
        Ice::Instrumentation::ObserverPtr observer;
    };

    const InstancePtr _instance;
    const IceInternal::ProtocolSupport _protocol;
    const bool _preferIPv6;
    bool _destroyed;
    std::deque<ResolveEntry> _queue;
    ObserverHelperT<Ice::Instrumentation::ThreadObserver> _observer;
    std::mutex _mutex;
    std::condition_variable _conditionVariable;
};

}

#endif
