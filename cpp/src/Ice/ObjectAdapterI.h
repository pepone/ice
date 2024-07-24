//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_OBJECT_ADAPTER_I_H
#define ICE_OBJECT_ADAPTER_I_H

#include "ConnectionFactoryF.h"
#include "ConnectionI.h"
#include "EndpointIF.h"
#include "Ice/BuiltinSequences.h"
#include "Ice/CommunicatorF.h"
#include "Ice/InstanceF.h"
#include "Ice/ObjectAdapter.h"
#include "Ice/ObjectF.h"
#include "Ice/Proxy.h"
#include "Ice/SSL/ServerAuthenticationOptions.h"
#include "LocatorInfoF.h"
#include "ObjectAdapterFactoryF.h"
#include "RouterInfoF.h"
#include "ServantManagerF.h"
#include "ThreadPoolF.h"

#ifdef __APPLE__
#    include <dispatch/dispatch.h>
#endif

#include <list>
#include <mutex>
#include <optional>
#include <stack>

namespace IceInternal
{
    class CommunicatorFlushBatchAsync;
    using CommunicatorFlushBatchAsyncPtr = std::shared_ptr<CommunicatorFlushBatchAsync>;
}

namespace Ice
{
    class ObjectAdapterI final : public ObjectAdapter, public std::enable_shared_from_this<ObjectAdapterI>
    {
    public:
        std::string getName() const noexcept final;

        CommunicatorPtr getCommunicator() const noexcept final;

        void activate() final;
        void hold() final;
        void waitForHold() final;
        void deactivate() noexcept;
        void waitForDeactivate() noexcept final;
        bool isDeactivated() const noexcept final;
        void destroy() noexcept final;

        ObjectAdapterPtr use(std::function<ObjectPtr(ObjectPtr)> middlewareFactory) final;

        ObjectPrx _add(const ObjectPtr&, const Identity&) final;
        ObjectPrx _addFacet(const ObjectPtr&, const Identity&, const std::string&) final;
        ObjectPrx _addWithUUID(const ObjectPtr&) final;
        ObjectPrx _addFacetWithUUID(const ObjectPtr&, const std::string&) final;
        void addDefaultServant(const ObjectPtr&, const std::string&) final;
        ObjectPtr remove(const Identity&) final;
        ObjectPtr removeFacet(const Identity&, const std::string&) final;
        FacetMap removeAllFacets(const Identity&) final;
        ObjectPtr removeDefaultServant(const std::string&) final;
        ObjectPtr find(const Identity&) const final;
        ObjectPtr findFacet(const Identity&, const std::string&) const final;
        FacetMap findAllFacets(const Identity&) const final;
        ObjectPtr findByProxy(const ObjectPrx&) const final;
        ObjectPtr findDefaultServant(const std::string&) const final;
        void addServantLocator(const ServantLocatorPtr&, const std::string&) final;
        ServantLocatorPtr removeServantLocator(const std::string&) final;
        ServantLocatorPtr findServantLocator(const std::string&) const final;

        const ObjectPtr& dispatchPipeline() const noexcept final;

        ObjectPrx _createProxy(const Identity&) const final;
        ObjectPrx _createDirectProxy(const Identity&) const final;
        ObjectPrx _createIndirectProxy(const Identity&) const final;

        void setLocator(const std::optional<LocatorPrx>&) final;
        std::optional<LocatorPrx> getLocator() const noexcept;
        EndpointSeq getEndpoints() const noexcept;

        void refreshPublishedEndpoints() final;
        EndpointSeq getPublishedEndpoints() const noexcept;
        void setPublishedEndpoints(const EndpointSeq&) final;

#ifdef __APPLE__
        dispatch_queue_t getDispatchQueue() const final;
#endif

        bool isLocal(const IceInternal::ReferencePtr&) const;

        void flushAsyncBatchRequests(const IceInternal::CommunicatorFlushBatchAsyncPtr&, CompressBatch);

        void updateConnectionObservers();
        void updateThreadObservers();

        void incDirectCount();
        void decDirectCount();

        IceInternal::ThreadPoolPtr getThreadPool() const;
        void setAdapterOnConnection(const ConnectionIPtr&);
        size_t messageSizeMax() const { return _messageSizeMax; }

        ObjectAdapterI(
            const IceInternal::InstancePtr&,
            const CommunicatorPtr&,
            const IceInternal::ObjectAdapterFactoryPtr&,
            const std::string&,
            bool,
            const std::optional<SSL::ServerAuthenticationOptions>&);
        virtual ~ObjectAdapterI();

        const std::optional<SSL::ServerAuthenticationOptions>& serverAuthenticationOptions() const noexcept
        {
            return _serverAuthenticationOptions;
        }

    private:
        void initialize(std::optional<RouterPrx>);
        friend class IceInternal::ObjectAdapterFactory;

        ObjectPrx newProxy(const Identity&, const std::string&) const;
        ObjectPrx newDirectProxy(const Identity&, const std::string&) const;
        ObjectPrx newIndirectProxy(const Identity&, const std::string&, const std::string&) const;
        void checkForDeactivation() const;
        std::vector<IceInternal::EndpointIPtr> parseEndpoints(const std::string&, bool) const;
        std::vector<IceInternal::EndpointIPtr> computePublishedEndpoints();
        void updateLocatorRegistry(const IceInternal::LocatorInfoPtr&, const std::optional<ObjectPrx>&);
        bool filterProperties(StringSeq&);

        enum State
        {
            StateUninitialized,
            StateHeld,
            StateActivating,
            StateActive,
            StateDeactivating,
            StateDeactivated,
            StateDestroying,
            StateDestroyed
        };
        State _state;
        IceInternal::InstancePtr _instance;
        CommunicatorPtr _communicator;
        IceInternal::ObjectAdapterFactoryPtr _objectAdapterFactory;
        IceInternal::ThreadPoolPtr _threadPool;
        const IceInternal::ServantManagerPtr _servantManager;

        mutable ObjectPtr _dispatchPipeline;
        mutable std::stack<std::function<ObjectPtr(ObjectPtr)>> _middlewareFactoryStack;

        const std::string _name;
        const std::string _id;
        const std::string _replicaGroupId;
        IceInternal::ReferencePtr _reference;
        std::vector<IceInternal::IncomingConnectionFactoryPtr> _incomingConnectionFactories;
        IceInternal::RouterInfoPtr _routerInfo;
        std::vector<IceInternal::EndpointIPtr> _publishedEndpoints;
        IceInternal::LocatorInfoPtr _locatorInfo;
        int _directCount; // The number of direct proxies dispatching on this object adapter.
        bool _noConfig;
        size_t _messageSizeMax;
        mutable std::recursive_mutex _mutex;
        std::condition_variable_any _conditionVariable;
        const std::optional<SSL::ServerAuthenticationOptions> _serverAuthenticationOptions;
    };
}

#endif
