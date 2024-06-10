//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_PROXY_H
#define ICE_PROXY_H

#include "BatchRequestQueueF.h"
#include "CommunicatorF.h"
#include "Current.h"
#include "EndpointF.h"
#include "EndpointSelectionType.h"
#include "Ice/BuiltinSequences.h"
#include "LocalException.h"
#include "ReferenceF.h"
#include "RequestHandlerF.h"

#include <future>
#include <iosfwd>
#include <optional>
#include <string_view>

namespace IceInternal
{
    class ProxyGetConnection;
    class ProxyFlushBatchAsync;

    template<typename T> class OutgoingAsyncT;
}

namespace Ice
{
    /** Marker value used to indicate that no explicit context was passed to a proxy invocation. */
    ICE_API extern const Context noExplicitContext;

    class LocatorPrx;
    class ObjectPrx;
    class OutputStream;
    class RouterPrx;

    /**
     * Helper template that supplies typed proxy factory functions.
     * \headerfile Ice/Ice.h
     */
    template<typename Prx, typename... Bases> class Proxy : public virtual Bases...
    {
    public:
        /**
         * The arrow operator.
         * @return A pointer to this object.
         * @remark This operator is provided for aesthetics and for compatibility reasons. A proxy is not pointer-like:
         * it does not hold a value and can't be null. Calling `greeter->greet("bob")` is 100% equivalent to calling
         * `greeter.greet("bob")` when greeter is a GreeterPrx proxy. An advantage of the arrow syntax is it's the same
         * whether greeter is a GreeterPrx, an optional<GreeterPrx>, or a smart pointer like in earlier versions of Ice.
         * It's also aesthetically pleasing when making invocations: the proxy appears like a pointer to the remote
         * object.
         */
        const Prx* operator->() const { return &asPrx(); }

        // We don't provide the non-const operator-> because only the assignment operators can modify the proxy.

        /**
         * Obtains a proxy that is identical to this proxy, except for the adapter ID.
         * @param id The adapter ID for the new proxy.
         * @return A proxy with the new adapter ID.
         */
        Prx ice_adapterId(std::string id) const { return fromReference(asPrx()._adapterId(std::move(id))); }

        /**
         * Obtains a proxy that is identical to this proxy, but uses batch datagram invocations.
         * @return A proxy that uses batch datagram invocations.
         */
        Prx ice_batchDatagram() const { return fromReference(asPrx()._batchDatagram()); }

        /**
         * Obtains a proxy that is identical to this proxy, but uses batch oneway invocations.
         * @return A proxy that uses batch oneway invocations.
         */
        Prx ice_batchOneway() const { return fromReference(asPrx()._batchOneway()); }

        /**
         * Obtains a proxy that is identical to this proxy, except for collocation optimization.
         * @param b True if the new proxy enables collocation optimization, false otherwise.
         * @return A proxy with the specified collocation optimization.
         */
        Prx ice_collocationOptimized(bool b) const { return fromReference(asPrx()._collocationOptimized(b)); }

        /**
         * Obtains a proxy that is identical to this proxy, except for its compression setting which
         * overrides the compression setting from the proxy endpoints.
         * @param b True enables compression for the new proxy, false disables compression.
         * @return A proxy with the specified compression override setting.
         */
        Prx ice_compress(bool b) const { return fromReference(asPrx()._compress(b)); }

        /**
         * Obtains a proxy that is identical to this proxy, except for connection caching.
         * @param b True if the new proxy should cache connections, false otherwise.
         * @return A proxy with the specified caching policy.
         */
        Prx ice_connectionCached(bool b) const { return fromReference(asPrx()._connectionCached(b)); }

        /**
         * Obtains a proxy that is identical to this proxy, except for its connection ID.
         * @param id The connection ID for the new proxy. An empty string removes the
         * connection ID.
         * @return A proxy with the specified connection ID.
         */
        Prx ice_connectionId(std::string id) const { return fromReference(asPrx()._connectionId(std::move(id))); }

        /**
         * Obtains a proxy that is identical to this proxy, except for the per-proxy context.
         * @param context The context for the new proxy.
         * @return A proxy with the new per-proxy context.
         */
        Prx ice_context(Context context) const { return fromReference(asPrx()._context(std::move(context))); }

        /**
         * Obtains a proxy that is identical to this proxy, but uses datagram invocations.
         * @return A proxy that uses datagram invocations.
         */
        Prx ice_datagram() const { return fromReference(asPrx()._datagram()); }

        /**
         * Obtains a proxy that is identical to this proxy, except for the encoding used to marshal
         * parameters.
         * @param version The encoding version to use to marshal request parameters.
         * @return A proxy with the specified encoding version.
         */
        Prx ice_encodingVersion(EncodingVersion version) const
        {
            return fromReference(asPrx()._encodingVersion(std::move(version)));
        }

        /**
         * Obtains a proxy that is identical to this proxy, except for the endpoint selection policy.
         * @param type The new endpoint selection policy.
         * @return A proxy with the specified endpoint selection policy.
         */
        Prx ice_endpointSelection(EndpointSelectionType type) const
        {
            return fromReference(asPrx()._endpointSelection(type));
        }

        /**
         * Obtains a proxy that is identical to this proxy, except for the endpoints.
         * @param endpoints The endpoints for the new proxy.
         * @return A proxy with the new endpoints.
         */
        Prx ice_endpoints(EndpointSeq endpoints) const
        {
            return fromReference(asPrx()._endpoints(std::move(endpoints)));
        }

        /**
         * Obtains a proxy that is identical to this proxy, except it's a fixed proxy bound
         * the given connection.
         * @param connection The fixed proxy connection.
         * @return A fixed proxy bound to the given connection.
         */
        Prx ice_fixed(ConnectionPtr connection) const { return fromReference(asPrx()._fixed(std::move(connection))); }

        /**
         * Obtains a proxy that is identical to this proxy, except for the invocation timeout.
         * @param timeout The new invocation timeout (in milliseconds).
         * @return A proxy with the new timeout.
         */
        Prx ice_invocationTimeout(int timeout) const { return fromReference(asPrx()._invocationTimeout(timeout)); }

        /**
         * Obtains a proxy that is identical to this proxy, except for the locator.
         * @param locator The locator for the new proxy.
         * @return A proxy with the specified locator.
         */
        Prx ice_locator(const std::optional<LocatorPrx>& locator) const
        {
            return fromReference(asPrx()._locator(locator));
        }

        /**
         * Obtains a proxy that is identical to this proxy, except for the locator cache timeout.
         * @param timeout The new locator cache timeout (in seconds).
         * @return A proxy with the new timeout.
         */
        Prx ice_locatorCacheTimeout(int timeout) const { return fromReference(asPrx()._locatorCacheTimeout(timeout)); }

        /**
         * Obtains a proxy that is identical to this proxy, but uses oneway invocations.
         * @return A proxy that uses oneway invocations.
         */
        Prx ice_oneway() const { return fromReference(asPrx()._oneway()); }

        /**
         * Obtains a proxy that is identical to this proxy, except for its endpoint selection policy.
         * @param b If true, the new proxy will use secure endpoints for invocations and only use
         * insecure endpoints if an invocation cannot be made via secure endpoints. If false, the
         * proxy prefers insecure endpoints to secure ones.
         * @return A proxy with the specified selection policy.
         */
        Prx ice_preferSecure(bool b) const { return fromReference(asPrx()._preferSecure(b)); }

        /**
         * Obtains a proxy that is identical to this proxy, except for the router.
         * @param router The router for the new proxy.
         * @return A proxy with the specified router.
         */
        Prx ice_router(const std::optional<RouterPrx>& router) const
        {
            return fromReference(asPrx()._router(std::move(router)));
        }

        /**
         * Obtains a proxy that is identical to this proxy, except for how it selects endpoints.
         * @param b If true, only endpoints that use a secure transport are used by the new proxy.
         * If false, the returned proxy uses both secure and insecure endpoints.
         * @return A proxy with the specified security policy.
         */
        Prx ice_secure(bool b) const { return fromReference(asPrx()._secure(b)); }

        /**
         * Obtains a proxy that is identical to this proxy, but uses twoway invocations.
         * @return A proxy that uses twoway invocations.
         */
        Prx ice_twoway() const { return fromReference(asPrx()._twoway()); }

    protected:
        // This constructor never initializes the base classes since they are all virtual and Proxy is never the most
        // derived class.
        Proxy() = default;

        // The copy constructor and assignment operators are to keep GCC happy.
        Proxy(const Proxy&) noexcept = default;
        Proxy& operator=(const Proxy&) noexcept { return *this; }
        Proxy& operator=(Proxy&&) noexcept { return *this; }

    private:
        Prx fromReference(IceInternal::ReferencePtr&& ref) const
        {
            const Prx& self = asPrx();
            return ref == self._reference ? self : Prx::_fromReference(std::move(ref));
        }

        const Prx& asPrx() const { return *static_cast<const Prx*>(this); }
    };

    /**
     * Base class of all object proxies.
     * \headerfile Ice/Ice.h
     */
    class ICE_API ObjectPrx : public Proxy<ObjectPrx>
    {
    public:
        ObjectPrx(const ObjectPrx& other) noexcept = default;
        ObjectPrx(ObjectPrx&&) noexcept = default;

        ObjectPrx(const Ice::CommunicatorPtr& communicator, std::string_view proxyString);

        virtual ~ObjectPrx() = default;

        ObjectPrx& operator=(const ObjectPrx& rhs) noexcept = default;
        ObjectPrx& operator=(ObjectPrx&& rhs) noexcept = default;

        /**
         * Tests whether this object supports a specific Slice interface.
         * @param typeId The type ID of the Slice interface to test against.
         * @param context The context map for the invocation.
         * @return true if the target object has the interface
         * specified by id or derives from the interface specified by id.
         */
        bool ice_isA(std::string_view typeId, const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Tests whether this object supports a specific Slice interface.
         * @param typeId The type ID of the Slice interface to test against.
         * @param response The response callback.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @param context The context map for the invocation.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_isAAsync(
            std::string_view typeId,
            std::function<void(bool)> response,
            std::function<void(std::exception_ptr)> ex = nullptr,
            std::function<void(bool)> sent = nullptr,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Tests whether this object supports a specific Slice interface.
         * @param typeId The type ID of the Slice interface to test against.
         * @param context The context map for the invocation.
         * @return The future object for the invocation.
         */
        std::future<bool>
        ice_isAAsync(std::string_view typeId, const Ice::Context& context = Ice::noExplicitContext) const;

        /// \cond INTERNAL
        void _iceI_isA(const std::shared_ptr<IceInternal::OutgoingAsyncT<bool>>&, std::string_view, const Ice::Context&)
            const;
        /// \endcond

        /**
         * Tests whether the target object of this proxy can be reached.
         * @param context The context map for the invocation.
         */
        void ice_ping(const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Tests whether the target object of this proxy can be reached.
         * @param response The response callback.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @param context The context map for the invocation.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_pingAsync(
            std::function<void()> response,
            std::function<void(std::exception_ptr)> ex = nullptr,
            std::function<void(bool)> sent = nullptr,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Tests whether the target object of this proxy can be reached.
         * @param context The context map for the invocation.
         * @return The future object for the invocation.
         */
        std::future<void> ice_pingAsync(const Ice::Context& context = Ice::noExplicitContext) const;

        /// \cond INTERNAL
        void _iceI_ping(const std::shared_ptr<IceInternal::OutgoingAsyncT<void>>&, const Ice::Context&) const;
        /// \endcond

        /**
         * Returns the Slice type IDs of the interfaces supported by the target object of this proxy.
         * @param context The context map for the invocation.
         * @return The Slice type IDs of the interfaces supported by the target object, in alphabetical order.
         */
        std::vector<std::string> ice_ids(const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Returns the Slice type IDs of the interfaces supported by the target object of this proxy.
         * @param response The response callback.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @param context The context map for the invocation.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_idsAsync(
            std::function<void(std::vector<std::string>)> response,
            std::function<void(std::exception_ptr)> ex = nullptr,
            std::function<void(bool)> sent = nullptr,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Returns the Slice type IDs of the interfaces supported by the target object of this proxy.
         * @param context The context map for the invocation.
         * @return The future object for the invocation.
         */
        std::future<std::vector<std::string>> ice_idsAsync(const Ice::Context& context = Ice::noExplicitContext) const;

        /// \cond INTERNAL
        void _iceI_ids(
            const std::shared_ptr<IceInternal::OutgoingAsyncT<std::vector<std::string>>>&,
            const Ice::Context&) const;
        /// \endcond

        /**
         * Returns the Slice type ID of the most-derived interface supported by the target object of this proxy.
         * @param context The context map for the invocation.
         * @return The Slice type ID of the most-derived interface.
         */
        std::string ice_id(const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Returns the Slice type ID of the most-derived interface supported by the target object of this proxy.
         * @param response The response callback.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @param context The context map for the invocation.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_idAsync(
            std::function<void(std::string)> response,
            std::function<void(std::exception_ptr)> ex = nullptr,
            std::function<void(bool)> sent = nullptr,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Returns the Slice type ID of the most-derived interface supported by the target object of this proxy.
         * @param context The context map for the invocation.
         * @return The future object for the invocation.
         */
        std::future<std::string> ice_idAsync(const Ice::Context& context = Ice::noExplicitContext) const;

        /// \cond INTERNAL
        void _iceI_id(const std::shared_ptr<IceInternal::OutgoingAsyncT<std::string>>&, const Ice::Context&) const;
        /// \endcond

        /**
         * Invokes an operation dynamically.
         * @param operation The name of the operation to invoke.
         * @param mode The operation mode (normal or idempotent).
         * @param inParams An encapsulation containing the encoded in-parameters for the operation.
         * @param outParams An encapsulation containing the encoded results.
         * @param context The context map for the invocation.
         * @return True if the operation completed successfully, in which case outParams contains
         * the encoded out parameters. False if the operation raised a user exception, in which
         * case outParams contains the encoded user exception. If the operation raises a run-time
         * exception, it throws it directly.
         */
        bool ice_invoke(
            std::string_view operation,
            Ice::OperationMode mode,
            const std::vector<std::byte>& inParams,
            std::vector<std::byte>& outParams,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Invokes an operation dynamically.
         * @param operation The name of the operation to invoke.
         * @param mode The operation mode (normal or idempotent).
         * @param inParams An encapsulation containing the encoded in-parameters for the operation.
         * @param context The context map for the invocation.
         * @return The future object for the invocation.
         */
        std::future<std::tuple<bool, std::vector<std::byte>>> ice_invokeAsync(
            std::string_view operation,
            Ice::OperationMode mode,
            const std::vector<std::byte>& inParams,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Invokes an operation dynamically.
         * @param operation The name of the operation to invoke.
         * @param mode The operation mode (normal or idempotent).
         * @param inParams An encapsulation containing the encoded in-parameters for the operation.
         * @param response The response callback.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @param context The context map for the invocation.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_invokeAsync(
            std::string_view operation,
            Ice::OperationMode mode,
            const std::vector<std::byte>& inParams,
            std::function<void(bool, std::vector<std::byte>)> response,
            std::function<void(std::exception_ptr)> ex = nullptr,
            std::function<void(bool)> sent = nullptr,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Invokes an operation dynamically.
         * @param operation The name of the operation to invoke.
         * @param mode The operation mode (normal or idempotent).
         * @param inParams An encapsulation containing the encoded in-parameters for the operation.
         * @param outParams An encapsulation containing the encoded results.
         * @param context The context map for the invocation.
         * @return True if the operation completed successfully, in which case outParams contains
         * the encoded out parameters. False if the operation raised a user exception, in which
         * case outParams contains the encoded user exception. If the operation raises a run-time
         * exception, it throws it directly.
         */
        bool ice_invoke(
            std::string_view operation,
            Ice::OperationMode mode,
            std::pair<const std::byte*, const std::byte*> inParams,
            std::vector<std::byte>& outParams,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Invokes an operation dynamically.
         * @param operation The name of the operation to invoke.
         * @param mode The operation mode (normal or idempotent).
         * @param inParams An encapsulation containing the encoded in-parameters for the operation.
         * @param context The context map for the invocation.
         * @return The future object for the invocation.
         */
        std::future<std::tuple<bool, std::vector<std::byte>>> ice_invokeAsync(
            std::string_view operation,
            Ice::OperationMode mode,
            std::pair<const std::byte*, const std::byte*> inParams,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Invokes an operation dynamically.
         * @param operation The name of the operation to invoke.
         * @param mode The operation mode (normal or idempotent).
         * @param inParams An encapsulation containing the encoded in-parameters for the operation.
         * @param response The response callback.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @param context The context map for the invocation.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_invokeAsync(
            std::string_view operation,
            Ice::OperationMode mode,
            std::pair<const std::byte*, const std::byte*> inParams,
            std::function<void(bool, std::pair<const std::byte*, const std::byte*>)> response,
            std::function<void(std::exception_ptr)> ex = nullptr,
            std::function<void(bool)> sent = nullptr,
            const Ice::Context& context = Ice::noExplicitContext) const;

        /**
         * Obtains the Connection for this proxy. If the proxy does not yet have an established connection,
         * it first attempts to create a connection.
         * @return The connection for this proxy.
         */
        Ice::ConnectionPtr ice_getConnection() const;

        /**
         * Obtains the Connection for this proxy. If the proxy does not yet have an established connection,
         * it first attempts to create a connection.
         * @param response The response callback.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_getConnectionAsync(
            std::function<void(Ice::ConnectionPtr)> response,
            std::function<void(std::exception_ptr)> ex = nullptr,
            std::function<void(bool)> sent = nullptr) const;

        /**
         * Obtains the Connection for this proxy. If the proxy does not yet have an established connection,
         * it first attempts to create a connection.
         * @return The future object for the invocation.
         */
        std::future<Ice::ConnectionPtr> ice_getConnectionAsync() const;

        /// \cond INTERNAL
        void _iceI_getConnection(const std::shared_ptr<IceInternal::ProxyGetConnection>&) const;
        /// \endcond

        /**
         * Obtains the cached Connection for this proxy. If the proxy does not yet have an established
         * connection, it does not attempt to create a connection.
         * @return The cached connection for this proxy, or nil if the proxy does not have
         * an established connection.
         */
        Ice::ConnectionPtr ice_getCachedConnection() const;

        /**
         * Flushes any pending batched requests for this communicator. The call blocks until the flush is complete.
         */
        void ice_flushBatchRequests() const;

        /**
         * Flushes asynchronously any pending batched requests for this communicator.
         * @param ex The exception callback.
         * @param sent The sent callback.
         * @return A function that can be called to cancel the invocation locally.
         */
        std::function<void()> ice_flushBatchRequestsAsync(
            std::function<void(std::exception_ptr)> ex,
            std::function<void(bool)> sent = nullptr) const;

        /**
         * Flushes asynchronously any pending batched requests for this communicator.
         * @return The future object for the invocation.
         */
        std::future<void> ice_flushBatchRequestsAsync() const;

        /**
         * Obtains the identity embedded in this proxy.
         * @return The identity of the target object.
         */
        Ice::Identity ice_getIdentity() const;

        /**
         * Obtains a proxy that is identical to this proxy, except for the identity.
         * @param id The identity for the new proxy.
         * @return A proxy with the new identity.
         */
        ObjectPrx ice_identity(Ice::Identity id) const;

        /**
         * Obtains the per-proxy context for this proxy.
         * @return The per-proxy context.
         */
        Ice::Context ice_getContext() const;

        /**
         * Obtains the facet for this proxy.
         * @return The facet for this proxy. If the proxy uses the default facet, the return value is the empty string.
         */
        const std::string& ice_getFacet() const;

        /**
         * Obtains a proxy that is identical to this proxy, except for the facet.
         * @param facet The facet for the new proxy.
         * @return A proxy with the new facet.
         */
        Ice::ObjectPrx ice_facet(std::string facet) const;

        /**
         * Obtains the adapter ID for this proxy.
         * @return The adapter ID. If the proxy does not have an adapter ID, the return value is the empty string.
         */
        std::string ice_getAdapterId() const;

        /**
         * Obtains the endpoints used by this proxy.
         * @return The endpoints used by this proxy.
         */
        Ice::EndpointSeq ice_getEndpoints() const;

        /**
         * Obtains the locator cache timeout of this proxy.
         * @return The locator cache timeout value (in seconds).
         */
        std::int32_t ice_getLocatorCacheTimeout() const;

        /**
         * Determines whether this proxy caches connections.
         * @return True if this proxy caches connections, false otherwise.
         */
        bool ice_isConnectionCached() const;

        /**
         * Obtains the endpoint selection policy for this proxy (randomly or ordered).
         * @return The endpoint selection policy.
         */
        Ice::EndpointSelectionType ice_getEndpointSelection() const;

        /**
         * Determines whether this proxy uses only secure endpoints.
         * @return True if this proxy communicates only via secure endpoints, false otherwise.
         */
        bool ice_isSecure() const;

        /**
         * Obtains the encoding version used to marshal request parameters.
         * @return The encoding version.
         */
        Ice::EncodingVersion ice_getEncodingVersion() const;

        /**
         * Determines whether this proxy prefers secure endpoints.
         * @return True if the proxy always attempts to invoke via secure endpoints before it
         * attempts to use insecure endpoints, false otherwise.
         */
        bool ice_isPreferSecure() const;

        /**
         * Obtains the router for this proxy.
         * @return The router for the proxy. If no router is configured for the proxy, the return value
         * is nullopt.
         */
        std::optional<RouterPrx> ice_getRouter() const;

        /**
         * Obtains the locator for this proxy.
         * @return The locator for this proxy. If no locator is configured, the return value is nullopt.
         */
        std::optional<LocatorPrx> ice_getLocator() const;

        /**
         * Determines whether this proxy uses collocation optimization.
         * @return True if the proxy uses collocation optimization, false otherwise.
         */
        bool ice_isCollocationOptimized() const;

        /**
         * Obtains the invocation timeout of this proxy.
         * @return The invocation timeout value (in milliseconds).
         */
        std::int32_t ice_getInvocationTimeout() const;

        /**
         * Determines whether this proxy uses twoway invocations.
         * @return True if this proxy uses twoway invocations, false otherwise.
         */
        bool ice_isTwoway() const;

        /**
         * Determines whether this proxy uses oneway invocations.
         * @return True if this proxy uses oneway invocations, false otherwise.
         */
        bool ice_isOneway() const;

        /**
         * Determines whether this proxy uses batch oneway invocations.
         * @return True if this proxy uses batch oneway invocations, false otherwise.
         */
        bool ice_isBatchOneway() const;

        /**
         * Determines whether this proxy uses datagram invocations.
         * @return True if this proxy uses datagram invocations, false otherwise.
         */
        bool ice_isDatagram() const;

        /**
         * Determines whether this proxy uses batch datagram invocations.
         * @return True if this proxy uses batch datagram invocations, false otherwise.
         */
        bool ice_isBatchDatagram() const;

        /**
         * Obtains the compression override setting of this proxy.
         * @return The compression override setting. If nullopt is returned, no override is set. Otherwise, true
         * if compression is enabled, false otherwise.
         */
        std::optional<bool> ice_getCompress() const;

        /**
         * Obtains the connection ID of this proxy.
         * @return The connection ID.
         */
        std::string ice_getConnectionId() const;

        /**
         * Determines whether this proxy is a fixed proxy.
         * @return True if this proxy is a fixed proxy, false otherwise.
         */
        bool ice_isFixed() const;

        /**
         * Returns the Slice type ID associated with this type.
         * @return The Slice type ID.
         */
        static std::string_view ice_staticId() noexcept;

        /**
         * Obtains the communicator that created this proxy.
         * @return The communicator that created this proxy.
         */
        Ice::CommunicatorPtr ice_getCommunicator() const;

        /**
         * Obtains a stringified version of this proxy.
         * @return A stringified proxy.
         */
        std::string ice_toString() const;

        /// \cond INTERNAL
        void _iceI_flushBatchRequests(const std::shared_ptr<IceInternal::ProxyFlushBatchAsync>&) const;

        static ObjectPrx _fromReference(IceInternal::ReferencePtr ref) { return ObjectPrx(std::move(ref)); }

        const IceInternal::ReferencePtr& _getReference() const { return _reference; }
        const IceInternal::RequestHandlerCachePtr& _getRequestHandlerCache() const { return _requestHandlerCache; }

        void _checkTwowayOnly(std::string_view) const;

        size_t _hash() const noexcept;

        void _write(OutputStream&) const;
        /// \endcond

    protected:
        /// \cond INTERNAL
        // This constructor is never called; it allows Proxy's default constructor to compile.
        ObjectPrx() = default;

        // The constructor used by _fromReference.
        explicit ObjectPrx(IceInternal::ReferencePtr&&);
        /// \endcond

    private:
        template<typename Prx, typename... Bases> friend class Proxy;

        // Gets a reference with the specified setting; returns _reference if the setting is already set.
        IceInternal::ReferencePtr _adapterId(std::string) const;
        IceInternal::ReferencePtr _batchDatagram() const;
        IceInternal::ReferencePtr _batchOneway() const;
        IceInternal::ReferencePtr _collocationOptimized(bool) const;
        IceInternal::ReferencePtr _compress(bool) const;
        IceInternal::ReferencePtr _connectionCached(bool) const;
        IceInternal::ReferencePtr _connectionId(std::string) const;
        IceInternal::ReferencePtr _context(Context) const;
        IceInternal::ReferencePtr _datagram() const;
        IceInternal::ReferencePtr _encodingVersion(EncodingVersion) const;
        IceInternal::ReferencePtr _endpointSelection(EndpointSelectionType) const;
        IceInternal::ReferencePtr _endpoints(EndpointSeq) const;
        IceInternal::ReferencePtr _fixed(ConnectionPtr) const;
        IceInternal::ReferencePtr _invocationTimeout(int) const;
        IceInternal::ReferencePtr _locator(const std::optional<LocatorPrx>&) const;
        IceInternal::ReferencePtr _locatorCacheTimeout(int) const;
        IceInternal::ReferencePtr _oneway() const;
        IceInternal::ReferencePtr _preferSecure(bool) const;
        IceInternal::ReferencePtr _router(const std::optional<RouterPrx>&) const;
        IceInternal::ReferencePtr _secure(bool) const;
        IceInternal::ReferencePtr _timeout(int) const;
        IceInternal::ReferencePtr _twoway() const;

        // Only the assignment operators can change these fields. All other member functions must be const.
        IceInternal::ReferencePtr _reference;
        IceInternal::RequestHandlerCachePtr _requestHandlerCache;
    };
}

namespace std
{
    /// Specialization of std::hash for Ice::ObjectPrx.
    template<> struct hash<Ice::ObjectPrx>
    {
        std::size_t operator()(const Ice::ObjectPrx& p) const noexcept { return p._hash(); }
    };
}

#endif
