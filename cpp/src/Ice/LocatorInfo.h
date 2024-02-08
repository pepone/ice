//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_LOCATOR_INFO_H
#define ICE_LOCATOR_INFO_H

#include <IceUtil/Shared.h>
#include <IceUtil/Time.h>
#include <Ice/LocatorInfoF.h>
#include <Ice/LocatorF.h>
#include <Ice/ReferenceF.h>
#include <Ice/Identity.h>
#include <Ice/EndpointIF.h>
#include <Ice/PropertiesF.h>
#include <Ice/Version.h>

#include <mutex>
#include <condition_variable>

namespace IceInternal
{

class LocatorManager final
{
public:

    LocatorManager(const Ice::PropertiesPtr&);

    void destroy();

    //
    // Returns locator info for a given locator. Automatically creates
    // the locator info if it doesn't exist yet.
    //
    LocatorInfoPtr get(const Ice::LocatorPrxPtr&);

private:

    const bool _background;

    using LocatorInfoTable = std::map<std::shared_ptr<Ice::LocatorPrx>,
                                      LocatorInfoPtr,
                                      Ice::TargetCompare<std::shared_ptr<Ice::LocatorPrx>, std::less>>;
    LocatorInfoTable _table;
    LocatorInfoTable::iterator _tableHint;

    std::map<std::pair<Ice::Identity, Ice::EncodingVersion>, LocatorTablePtr> _locatorTables;
    std::mutex _mutex;
};

class LocatorTable final
{
public:

    LocatorTable();

    void clear();

    bool getAdapterEndpoints(const std::string&, int, ::std::vector<EndpointIPtr>&);
    void addAdapterEndpoints(const std::string&, const ::std::vector<EndpointIPtr>&);
    ::std::vector<EndpointIPtr> removeAdapterEndpoints(const std::string&);

    bool getObjectReference(const Ice::Identity&, int, ReferencePtr&);
    void addObjectReference(const Ice::Identity&, const ReferencePtr&);
    ReferencePtr removeObjectReference(const Ice::Identity&);

private:

    bool checkTTL(const IceUtil::Time&, int) const;

    std::map<std::string, std::pair<IceUtil::Time, std::vector<EndpointIPtr> > > _adapterEndpointsMap;
    std::map<Ice::Identity, std::pair<IceUtil::Time, ReferencePtr> > _objectMap;
    std::mutex _mutex;
};

class LocatorInfo final : public std::enable_shared_from_this<LocatorInfo>
{
public:

    class GetEndpointsCallback
    {
    public:

        virtual void setEndpoints(const std::vector<EndpointIPtr>&, bool) = 0;
        virtual void setException(const Ice::LocalException&) = 0;
    };
    using GetEndpointsCallbackPtr = std::shared_ptr<GetEndpointsCallback>;

    class RequestCallback final
    {
    public:

        RequestCallback(const ReferencePtr&, int, const GetEndpointsCallbackPtr&);

        void response(const LocatorInfoPtr&, const Ice::ObjectPrxPtr&);
        void exception(const LocatorInfoPtr&, const Ice::Exception&);

    private:

        const ReferencePtr _reference;
        const int _ttl;
        const GetEndpointsCallbackPtr _callback;
    };
    using RequestCallbackPtr = std::shared_ptr<RequestCallback>;

    class Request
    {
    public:

        void addCallback(const ReferencePtr&, const ReferencePtr&, int, const GetEndpointsCallbackPtr&);

        void response(const Ice::ObjectPrxPtr&);
        void exception(const Ice::Exception&);

    protected:

        Request(const LocatorInfoPtr&, const ReferencePtr&);

        virtual void send() = 0;

        const LocatorInfoPtr _locatorInfo;
        const ReferencePtr _reference;

    private:

        std::mutex _mutex;
        std::vector<RequestCallbackPtr> _callbacks;
        std::vector<ReferencePtr> _wellKnownRefs;
        bool _sent;
        bool _response;
        Ice::ObjectPrxPtr _proxy;
        std::unique_ptr<Ice::Exception> _exception;
    };
    using RequestPtr = std::shared_ptr<Request>;

    LocatorInfo(const Ice::LocatorPrxPtr&, const LocatorTablePtr&, bool);

    void destroy();

    bool operator==(const LocatorInfo&) const;
    bool operator<(const LocatorInfo&) const;

    const Ice::LocatorPrxPtr& getLocator() const
    {
        //
        // No mutex lock necessary, _locator is immutable.
        //
        return _locator;
    }
    Ice::LocatorRegistryPrxPtr getLocatorRegistry();

    void getEndpoints(const ReferencePtr& ref, int ttl, const GetEndpointsCallbackPtr& cb)
    {
        getEndpoints(ref, 0, ttl, cb);
    }
    void getEndpoints(const ReferencePtr&, const ReferencePtr&, int, const GetEndpointsCallbackPtr&);

    void clearCache(const ReferencePtr&);

private:

    void getEndpointsException(const ReferencePtr&, const Ice::Exception&);
    void getEndpointsTrace(const ReferencePtr&, const std::vector<EndpointIPtr>&, bool);
    void trace(const std::string&, const ReferencePtr&, const std::vector<EndpointIPtr>&);
    void trace(const std::string&, const ReferencePtr&, const ReferencePtr&);

    RequestPtr getAdapterRequest(const ReferencePtr&);
    RequestPtr getObjectRequest(const ReferencePtr&);

    void finishRequest(const ReferencePtr&, const std::vector<ReferencePtr>&, const Ice::ObjectPrxPtr&, bool);
    friend class Request;
    friend class RequestCallback;

    const Ice::LocatorPrxPtr _locator;
    Ice::LocatorRegistryPrxPtr _locatorRegistry;
    const LocatorTablePtr _table;
    const bool _background;

    std::map<std::string, RequestPtr> _adapterRequests;
    std::map<Ice::Identity, RequestPtr> _objectRequests;
    std::mutex _mutex;
};

}

#endif
