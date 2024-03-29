//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `Communicator.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/**
 * The central object in Ice. One or more communicators can be instantiated for an Ice application. Communicator
 * instantiation is language-specific, and not specified in Slice code.
 *
 * @see Logger
 * @see ObjectAdapter
 * @see Properties
 * @see ValueFactory
 **/
public interface Communicator extends java.lang.AutoCloseable
{
    /**
     * Destroy the communicator. This Java-only method overrides close in java.lang.AutoCloseable and does not throw
     * any exception.
     *
     * @see #destroy
     **/
    void close();

    /**
     * Destroy the communicator. This operation calls {@link #shutdown} implicitly. Calling {@link #destroy} cleans up
     * memory, and shuts down this communicator's client functionality and destroys all object adapters. Subsequent
     * calls to {@link #destroy} are ignored.
     *
     * @see #shutdown
     * @see ObjectAdapter#destroy
     **/
    void destroy();

    /**
     * Shuts down this communicator's server functionality, which includes the deactivation of all object adapters.
     * Attempts to use a deactivated object adapter raise ObjectAdapterDeactivatedException. Subsequent calls to
     * shutdown are ignored.
     * After shutdown returns, no new requests are processed. However, requests that have been started before shutdown
     * was called might still be active. You can use {@link #waitForShutdown} to wait for the completion of all
     * requests.
     *
     * @see #destroy
     * @see #waitForShutdown
     * @see ObjectAdapter#deactivate
     **/
    void shutdown();

    /**
     * Wait until the application has called {@link #shutdown} (or {@link #destroy}). On the server side, this
     * operation blocks the calling thread until all currently-executing operations have completed. On the client
     * side, the operation simply blocks until another thread has called {@link #shutdown} or {@link #destroy}.
     * A typical use of this operation is to call it from the main thread, which then waits until some other thread
     * calls {@link #shutdown}. After shut-down is complete, the main thread returns and can do some cleanup work
     * before it finally calls {@link #destroy} to shut down the client functionality, and then exits the application.
     *
     * @see #shutdown
     * @see #destroy
     * @see ObjectAdapter#waitForDeactivate
     **/
    void waitForShutdown();

    /**
     * Check whether communicator has been shut down.
     * @return True if the communicator has been shut down; false otherwise.
     *
     * @see #shutdown
     **/
    boolean isShutdown();

    /**
     * Convert a stringified proxy into a proxy.
     * For example, <code>MyCategory/MyObject:tcp -h some_host -p 10000</code> creates a proxy that refers to the Ice
     * object having an identity with a name "MyObject" and a category "MyCategory", with the server running on host
     * "some_host", port 10000. If the stringified proxy does not parse correctly, the operation throws one of
     * ProxyParseException, EndpointParseException, or IdentityParseException. Refer to the Ice manual for a detailed
     * description of the syntax supported by stringified proxies.
     * @param str The stringified proxy to convert into a proxy.
     * @return The proxy, or nil if <code>str</code> is an empty string.
     *
     * @see #proxyToString
     **/
    ObjectPrx stringToProxy(String str);

    /**
     * Convert a proxy into a string.
     * @param obj The proxy to convert into a stringified proxy.
     * @return The stringified proxy, or an empty string if
     * <code>obj</code> is nil.
     *
     * @see #stringToProxy
     **/
    String proxyToString(ObjectPrx obj);

    /**
     * Convert a set of proxy properties into a proxy. The "base" name supplied in the <code>property</code> argument
     * refers to a property containing a stringified proxy, such as <code>MyProxy=id:tcp -h localhost -p 10000</code>.
     * Additional properties configure local settings for the proxy, such as <code>MyProxy.PreferSecure=1</code>. The
     * "Properties" appendix in the Ice manual describes each of the supported proxy properties.
     * @param property The base property name.
     * @return The proxy.
     **/
    ObjectPrx propertyToProxy(String property);

    /**
     * Convert a proxy to a set of proxy properties.
     * @param proxy The proxy.
     * @param property The base property name.
     * @return The property set.
     **/
    java.util.Map<java.lang.String, java.lang.String> proxyToProperty(ObjectPrx proxy, String property);

    /**
     * Convert an identity into a string.
     * @param ident The identity to convert into a string.
     * @return The "stringified" identity.
     **/
    String identityToString(Identity ident);

    /**
     * Create a new object adapter. The endpoints for the object adapter are taken from the property
     * <code><em>name</em>.Endpoints</code>.
     * It is legal to create an object adapter with the empty string as its name. Such an object adapter is accessible
     * via bidirectional connections or by collocated invocations that originate from the same communicator as is used
     * by the adapter. Attempts to create a named object adapter for which no configuration can be found raise
     * InitializationException.
     * @param name The object adapter name.
     * @return The new object adapter.
     *
     * @see #createObjectAdapterWithEndpoints
     * @see ObjectAdapter
     * @see Properties
     **/
    ObjectAdapter createObjectAdapter(String name);

    /**
     * Create a new object adapter. The endpoints for the object adapter are taken from the property
     * <code><em>name</em>.Endpoints</code>.
     * It is legal to create an object adapter with the empty string as its name. Such an object adapter is accessible
     * via bidirectional connections or by collocated invocations that originate from the same communicator as is used
     * by the adapter. Attempts to create a named object adapter for which no configuration can be found raise
     * InitializationException.
     * @param name The object adapter name.
     * @param sslEngineFactory The SSLEngineFactory responsible for creating {@link javax.net.ssl.SSLEngine} instances
     * used by the SSL server-side transport. This factory allows to programmatically configure the SSL server-side
     * transport. If secure endpoints are not used, or if SSL settings are fully defined by IceSSL configuration
     * properties, pass <code>null</code> to ignore this parameter. When provided, this factory takes precedence over
     * IceSSL property-based.
     * @return The new object adapter.
     *
     * @see #createObjectAdapterWithEndpoints
     * @see ObjectAdapter
     * @see Properties
     **/
    ObjectAdapter createObjectAdapter(String name, com.zeroc.IceSSL.SSLEngineFactory sslEngineFactory);

    /**
     * Create a new object adapter with endpoints. This operation sets the property
     * <code><em>name</em>.Endpoints</code>, and then calls {@link #createObjectAdapter}. It is provided as a
     * convenience function. Calling this operation with an empty name will result in a UUID being generated for the
     * name.
     * @param name The object adapter name.
     * @param endpoints The endpoints for the object adapter.
     * @return The new object adapter.
     *
     * @see #createObjectAdapter
     * @see ObjectAdapter
     * @see Properties
     **/
    ObjectAdapter createObjectAdapterWithEndpoints(String name, String endpoints);

    /**
     * Create a new object adapter with endpoints. This operation sets the property
     * <code><em>name</em>.Endpoints</code>, and then calls {@link #createObjectAdapter}. It is provided as a
     * convenience function. Calling this operation with an empty name will result in a UUID being generated for the
     * name.
     * @param name The object adapter name.
     * @param endpoints The endpoints for the object adapter.
     * @param sslEngineFactory The SSLEngineFactory responsible for creating {@link javax.net.ssl.SSLEngine} instances
     * used by the SSL server-side transport. This factory allows to programmatically configure the SSL server-side
     * transport. If secure endpoints are not used, or if SSL settings are fully defined by IceSSL configuration
     * properties, pass <code>null</code> to ignore this parameter. When provided, this factory takes precedence over
     * IceSSL property-based.
     * @return The new object adapter.
     *
     * @see #createObjectAdapter
     * @see ObjectAdapter
     * @see Properties
     **/
    ObjectAdapter createObjectAdapterWithEndpoints(
        String name,
        String endpoints, 
        com.zeroc.IceSSL.SSLEngineFactory sslEngineFactory);

    /**
     * Create a new object adapter with a router. This operation creates a routed object adapter.
     * Calling this operation with an empty name will result in a UUID being generated for the name.
     * @param name The object adapter name.
     * @param rtr The router.
     * @return The new object adapter.
     *
     * @see #createObjectAdapter
     * @see ObjectAdapter
     * @see Properties
     **/
    ObjectAdapter createObjectAdapterWithRouter(String name, RouterPrx rtr);

    /**
     * Get the implicit context associated with this communicator.
     * @return The implicit context associated with this communicator; returns null when the property Ice.ImplicitContext
     * is not set or is set to None.
     **/
    ImplicitContext getImplicitContext();

    /**
     * Get the properties for this communicator.
     * @return This communicator's properties.
     *
     * @see Properties
     **/
    Properties getProperties();

    /**
     * Get the logger for this communicator.
     * @return This communicator's logger.
     *
     * @see Logger
     **/
    Logger getLogger();

    /**
     * Get the observer resolver object for this communicator.
     * @return This communicator's observer resolver object.
     **/
    com.zeroc.Ice.Instrumentation.CommunicatorObserver getObserver();

    /**
     * Get the default router for this communicator.
     * @return The default router for this communicator.
     *
     * @see #setDefaultRouter
     * @see Router
     **/
    RouterPrx getDefaultRouter();

    /**
     * Set a default router for this communicator. All newly created proxies will use this default router. To disable
     * the default router, null can be used. Note that this operation has no effect on existing proxies.
     * You can also set a router for an individual proxy by calling the operation <code>ice_router</code> on the
     * proxy.
     * @param rtr The default router to use for this communicator.
     *
     * @see #getDefaultRouter
     * @see #createObjectAdapterWithRouter
     * @see Router
     **/
    void setDefaultRouter(RouterPrx rtr);

    /**
     * Get the default locator for this communicator.
     * @return The default locator for this communicator.
     *
     * @see #setDefaultLocator
     * @see Locator
     **/
    LocatorPrx getDefaultLocator();

    /**
     * Set a default Ice locator for this communicator. All newly created proxy and object adapters will use this
     * default locator. To disable the default locator, null can be used. Note that this operation has no effect on
     * existing proxies or object adapters.
     * You can also set a locator for an individual proxy by calling the operation <code>ice_locator</code> on the
     * proxy, or for an object adapter by calling {@link ObjectAdapter#setLocator} on the object adapter.
     * @param loc The default locator to use for this communicator.
     *
     * @see #getDefaultLocator
     * @see Locator
     * @see ObjectAdapter#setLocator
     **/
    void setDefaultLocator(LocatorPrx loc);

    /**
     * Get the plug-in manager for this communicator.
     * @return This communicator's plug-in manager.
     *
     * @see PluginManager
     **/
    PluginManager getPluginManager();

    /**
     * Get the value factory manager for this communicator.
     * @return This communicator's value factory manager.
     *
     * @see ValueFactoryManager
     **/
    ValueFactoryManager getValueFactoryManager();

    /**
     * Flush any pending batch requests for this communicator. This means all batch requests invoked on fixed proxies
     * for all connections associated with the communicator. Any errors that occur while flushing a connection are
     * ignored.
     * @param compress Specifies whether or not the queued batch requests should be compressed before being sent over
     * the wire.
     **/
    void flushBatchRequests(CompressBatch compress);

    /**
     * Flush any pending batch requests for this communicator. This means all batch requests invoked on fixed proxies
     * for all connections associated with the communicator. Any errors that occur while flushing a connection are
     * ignored.
     * @param compress Specifies whether or not the queued batch requests should be compressed before being sent over
     * the wire.
     * @return A future that will be completed when the invocation completes.
     **/
    java.util.concurrent.CompletableFuture<Void> flushBatchRequestsAsync(CompressBatch compress);

    /**
     * Add the Admin object with all its facets to the provided object adapter. If <code>Ice.Admin.ServerId</code> is
     * set and the provided object adapter has a {@link Locator}, createAdmin registers the Admin's Process facet with
     * the {@link Locator}'s {@link LocatorRegistry}. createAdmin must only be called once; subsequent calls raise
     * InitializationException.
     * @param adminAdapter The object adapter used to host the Admin object; if null and Ice.Admin.Endpoints is set,
     * create, activate and use the Ice.Admin object adapter.
     * @param adminId The identity of the Admin object.
     * @return A proxy to the main ("") facet of the Admin object. Never returns a null proxy.
     *
     * @see #getAdmin
     **/
    ObjectPrx createAdmin(ObjectAdapter adminAdapter, Identity adminId);

    /**
     * Get a proxy to the main facet of the Admin object. getAdmin also creates the Admin object and creates and
     * activates the Ice.Admin object adapter to host this Admin object if Ice.Admin.Enpoints is set. The identity of
     * the Admin object created by getAdmin is {value of Ice.Admin.InstanceName}/admin, or {UUID}/admin when
     * <code>Ice.Admin.InstanceName</code> is not set. If Ice.Admin.DelayCreation is 0 or not set, getAdmin is called
     * by the communicator initialization, after initialization of all plugins.
     * @return A proxy to the main ("") facet of the Admin object, or a null proxy if no Admin object is configured.
     *
     * @see #createAdmin
     **/
    ObjectPrx getAdmin();

    /**
     * Add a new facet to the Admin object. Adding a servant with a facet that is already registered throws
     * AlreadyRegisteredException.
     * @param servant The servant that implements the new Admin facet.
     * @param facet The name of the new Admin facet.
     **/
    void addAdminFacet(Object servant, String facet);

    /**
     * Remove the following facet to the Admin object. Removing a facet that was not previously registered throws
     * NotRegisteredException.
     * @param facet The name of the Admin facet.
     * @return The servant associated with this Admin facet.
     **/
    Object removeAdminFacet(String facet);

    /**
     * Returns a facet of the Admin object.
     * @param facet The name of the Admin facet.
     * @return The servant associated with this Admin facet, or null if no facet is registered with the given name.
     **/
    Object findAdminFacet(String facet);

    /**
     * Returns a map of all facets of the Admin object.
     * @return A collection containing all the facet names and servants of the Admin object.
     *
     * @see #findAdminFacet
     **/
    java.util.Map<java.lang.String, com.zeroc.Ice.Object> findAllAdminFacets();
}
