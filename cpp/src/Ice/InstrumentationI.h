//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_INSTRUMENTATION_I_H
#define ICE_INSTRUMENTATION_I_H

#include <Ice/MetricsObserverI.h>
#include <Ice/Connection.h>

namespace IceInternal
{
    template<typename T, typename O> class ObserverWithDelegateT : public Ice::MX::ObserverT<T>, public virtual O
    {
    public:
        typedef O ObserverType;
        typedef typename std::shared_ptr<O> ObserverPtrType;
        virtual void attach()
        {
            Ice::MX::ObserverT<T>::attach();
            if (_delegate)
            {
                _delegate->attach();
            }
        }

        virtual void detach()
        {
            Ice::MX::ObserverT<T>::detach();
            if (_delegate)
            {
                _delegate->detach();
            }
        }

        virtual void failed(const std::string& exceptionName)
        {
            Ice::MX::ObserverT<T>::failed(exceptionName);
            if (_delegate)
            {
                _delegate->failed(exceptionName);
            }
        }

        ObserverPtrType getDelegate() const { return _delegate; }

        void setDelegate(ObserverPtrType delegate) { _delegate = delegate; }

        template<typename ObserverImpl, typename ObserverMetricsType, typename ObserverPtrType>
        ObserverPtrType getObserverWithDelegate(
            const std::string& mapName,
            const Ice::MX::MetricsHelperT<ObserverMetricsType>& helper,
            const ObserverPtrType& del)
        {
            std::shared_ptr<ObserverImpl> obsv =
                Ice::MX::ObserverT<T>::template getObserver<ObserverImpl>(mapName, helper);
            if (obsv)
            {
                obsv->setDelegate(del);
                return obsv;
            }
            return del;
        }

    protected:
        ObserverPtrType _delegate;
    };

    template<typename T> class ObserverFactoryWithDelegateT : public Ice::MX::ObserverFactoryT<T>
    {
    public:
        ObserverFactoryWithDelegateT(const IceInternal::MetricsAdminIPtr& metrics, const std::string& name)
            : Ice::MX::ObserverFactoryT<T>(metrics, name)
        {
        }

        template<typename ObserverMetricsType, typename ObserverPtrType>
        ObserverPtrType
        getObserverWithDelegate(const Ice::MX::MetricsHelperT<ObserverMetricsType>& helper, const ObserverPtrType& del)
        {
            std::shared_ptr<T> obsv = Ice::MX::ObserverFactoryT<T>::getObserver(helper);
            if (obsv)
            {
                obsv->setDelegate(del);
                return obsv;
            }
            return del;
        }

        template<typename ObserverMetricsType, typename ObserverPtrType>
        ObserverPtrType getObserverWithDelegate(
            const Ice::MX::MetricsHelperT<ObserverMetricsType>& helper,
            const ObserverPtrType& del,
            const ObserverPtrType& old)
        {
            std::shared_ptr<T> obsv = Ice::MX::ObserverFactoryT<T>::getObserver(helper, old);
            if (obsv)
            {
                obsv->setDelegate(del);
                return obsv;
            }
            return del;
        }
    };

    template<typename Helper> void addEndpointAttributes(typename Helper::Attributes& attrs)
    {
        attrs.add("endpoint", &Helper::getEndpoint);

        attrs.add("endpointType", &Helper::getEndpointInfo, &Ice::EndpointInfo::type);
        attrs.add("endpointIsDatagram", &Helper::getEndpointInfo, &Ice::EndpointInfo::datagram);
        attrs.add("endpointIsSecure", &Helper::getEndpointInfo, &Ice::EndpointInfo::secure);
        attrs.add("endpointTimeout", &Helper::getEndpointInfo, &Ice::EndpointInfo::timeout);
        attrs.add("endpointCompress", &Helper::getEndpointInfo, &Ice::EndpointInfo::compress);

        attrs.add("endpointHost", &Helper::getEndpointInfo, &Ice::IPEndpointInfo::host);
        attrs.add("endpointPort", &Helper::getEndpointInfo, &Ice::IPEndpointInfo::port);
    }

    template<typename Helper> void addConnectionAttributes(typename Helper::Attributes& attrs)
    {
        attrs.add("incoming", &Helper::getConnectionInfo, &Ice::ConnectionInfo::incoming);
        attrs.add("adapterName", &Helper::getConnectionInfo, &Ice::ConnectionInfo::adapterName);
        attrs.add("connectionId", &Helper::getConnectionInfo, &Ice::ConnectionInfo::connectionId);

        attrs.add("localHost", &Helper::getConnectionInfo, &Ice::IPConnectionInfo::localAddress);
        attrs.add("localPort", &Helper::getConnectionInfo, &Ice::IPConnectionInfo::localPort);
        attrs.add("remoteHost", &Helper::getConnectionInfo, &Ice::IPConnectionInfo::remoteAddress);
        attrs.add("remotePort", &Helper::getConnectionInfo, &Ice::IPConnectionInfo::remotePort);

        attrs.add("mcastHost", &Helper::getConnectionInfo, &Ice::UDPConnectionInfo::mcastAddress);
        attrs.add("mcastPort", &Helper::getConnectionInfo, &Ice::UDPConnectionInfo::mcastPort);

        addEndpointAttributes<Helper>(attrs);
    }

    class ConnectionObserverI
        : public ObserverWithDelegateT<Ice::MX::ConnectionMetrics, Ice::Instrumentation::ConnectionObserver>
    {
    public:
        virtual void sentBytes(std::int32_t);
        virtual void receivedBytes(std::int32_t);
    };

    class ThreadObserverI : public ObserverWithDelegateT<Ice::MX::ThreadMetrics, Ice::Instrumentation::ThreadObserver>
    {
    public:
        virtual void stateChanged(Ice::Instrumentation::ThreadState, Ice::Instrumentation::ThreadState);
    };

    class DispatchObserverI
        : public ObserverWithDelegateT<Ice::MX::DispatchMetrics, Ice::Instrumentation::DispatchObserver>
    {
    public:
        virtual void userException();

        virtual void reply(std::int32_t);
    };

    class RemoteObserverI : public ObserverWithDelegateT<Ice::MX::RemoteMetrics, Ice::Instrumentation::RemoteObserver>
    {
    public:
        virtual void reply(std::int32_t);
    };

    class CollocatedObserverI
        : public ObserverWithDelegateT<Ice::MX::CollocatedMetrics, Ice::Instrumentation::CollocatedObserver>
    {
    public:
        virtual void reply(std::int32_t);
    };

    class InvocationObserverI
        : public ObserverWithDelegateT<Ice::MX::InvocationMetrics, Ice::Instrumentation::InvocationObserver>
    {
    public:
        virtual void retried();

        virtual void userException();

        virtual Ice::Instrumentation::RemoteObserverPtr
        getRemoteObserver(const Ice::ConnectionInfoPtr&, const Ice::EndpointPtr&, std::int32_t, std::int32_t);

        virtual Ice::Instrumentation::CollocatedObserverPtr
        getCollocatedObserver(const Ice::ObjectAdapterPtr&, std::int32_t, std::int32_t);
    };

    typedef ObserverWithDelegateT<Ice::MX::Metrics, Ice::Instrumentation::Observer> ObserverI;

    class ICE_API CommunicatorObserverI : public Ice::Instrumentation::CommunicatorObserver
    {
    public:
        CommunicatorObserverI(const Ice::InitializationData&);

        virtual void setObserverUpdater(const Ice::Instrumentation::ObserverUpdaterPtr&);

        virtual Ice::Instrumentation::ObserverPtr
        getConnectionEstablishmentObserver(const Ice::EndpointPtr&, const std::string&);

        virtual Ice::Instrumentation::ObserverPtr getEndpointLookupObserver(const Ice::EndpointPtr&);

        virtual Ice::Instrumentation::ConnectionObserverPtr getConnectionObserver(
            const Ice::ConnectionInfoPtr&,
            const Ice::EndpointPtr&,
            Ice::Instrumentation::ConnectionState,
            const Ice::Instrumentation::ConnectionObserverPtr&);

        virtual Ice::Instrumentation::ThreadObserverPtr getThreadObserver(
            const std::string&,
            const std::string&,
            Ice::Instrumentation::ThreadState,
            const Ice::Instrumentation::ThreadObserverPtr&);

        virtual Ice::Instrumentation::InvocationObserverPtr
        getInvocationObserver(const std::optional<Ice::ObjectPrx>&, std::string_view, const Ice::Context&);

        virtual Ice::Instrumentation::DispatchObserverPtr getDispatchObserver(const Ice::Current&, std::int32_t);

        const IceInternal::MetricsAdminIPtr& getFacet() const;

        void destroy();

    private:
        IceInternal::MetricsAdminIPtr _metrics;
        const Ice::Instrumentation::CommunicatorObserverPtr _delegate;

        ObserverFactoryWithDelegateT<ConnectionObserverI> _connections;
        ObserverFactoryWithDelegateT<DispatchObserverI> _dispatch;
        ObserverFactoryWithDelegateT<InvocationObserverI> _invocations;
        ObserverFactoryWithDelegateT<ThreadObserverI> _threads;
        ObserverFactoryWithDelegateT<ObserverI> _connects;
        ObserverFactoryWithDelegateT<ObserverI> _endpointLookups;
    };
    using CommunicatorObserverIPtr = std::shared_ptr<CommunicatorObserverI>;

};

#endif
