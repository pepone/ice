//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

declare module "ice" {
    namespace Ice {
        /**
         * The central object in Ice. One or more communicators can be instantiated for an Ice application.
         *
         * @see {@link Logger}
         * @see {@link ObjectAdapter}
         * @see {@link Properties}
         * @see {@link ValueFactory}
         */
        interface Communicator {
            /**
             * Destroy the communicator. This operation calls {@link Communicator#shutdown} implicitly. Calling {@link Communicator#destroy} cleans up
             * memory, and shuts down this communicator's client functionality and destroys all object adapters.
             *
             * @returns A promise that is resolved when the communicator has been destroyed.
             *
             * @see {@link shutdown}
             * @see {@link ObjectAdapter#destroy}
             */
            destroy(): Promise<void>;

            /**
             * Shuts down this communicator's server functionality, including the deactivation of all object adapters.
             *
             * After `shutdown` returns, no new requests are processed. However, requests that were started before
             * `shutdown` was called might still be active. You can use {@link Communicator#waitForShutdown} to wait for
             * the completion of all active requests.
             *
             * @see {@link destroy}
             * @see {@link waitForShutdown}
             * @see {@link ObjectAdapter#deactivate}
             */
            shutdown(): void;

            /**
             * Waits until the application has called {@link Communicator#shutdown} or {@link Communicator#destroy}.
             *
             * On the server side, the returned promise is resolved after all currently-executing operations have completed.
             * On the client side, the returned promise is resolved after {@link Communicator#shutdown} or
             * {@link Communicator#destroy} have been called.
             *
             * @returns A promise that is resolved when the communicator has been shut down.
             *
             * @see {@link shutdown}
             * @see {@link destroy}
             * @see {@link ObjectAdapter#waitForDeactivate}
             */
            waitForShutdown(): Promise<void>;

            /**
             * Checks whether the communicator has been shut down.
             *
             * @returns `true` if the communicator has been shut down; `false` otherwise.
             *
             * @see {@link shutdown}
             */
            isShutdown(): boolean;

            /**
             * Converts a proxy string into a proxy.
             *
             * @param proxyString - The proxy string to convert.
             *
             * @returns The proxy, or `null` if `proxyString` is an empty string.
             *
             * @see {@link proxyToString}
             */
            stringToProxy(proxyString: string): Ice.ObjectPrx;

            /**
             * Converts a proxy into a string.
             *
             * @param prx - The proxy to convert into a string.
             * @returns The string representation of the proxy, or an empty string if `prx` is `null`.
             */
            proxyToString(prx: Ice.ObjectPrx): string;

            /**
             * Converts a set of proxy properties into a proxy.
             *
             * The "base" name supplied in the `property` argument refers to a property containing a
             * stringified proxy, such as `MyProxy=id:tcp -h localhost -p 10000`. Additional properties
             * configure local settings for the proxy, such as `MyProxy.PreferSecure=1`. The "Properties"
             * appendix in the Ice manual describes each of the supported proxy properties.
             *
             * @param property - The base property name.
             * @returns The proxy created from the specified properties.
             */
            propertyToProxy(property: string): Ice.ObjectPrx;

            /**
             * Converts a proxy into a set of proxy properties.
             *
             * @param proxy - The proxy to convert.
             * @param property - The base property name to associate with the proxy.
             * @returns A dictionary containing the property set derived from the proxy.
             */
            proxyToProperty(proxy: Ice.ObjectPrx, property: string): PropertyDict;

            /**
             * Converts an identity into a string.
             *
             * @param identity - The identity to convert into a string.
             * @returns The string representation of the identity.
             */
            identityToString(identity: Identity): string;

            /**
             * Creates a new object adapter.
             *
             * The endpoints for the object adapter are taken from the property `<name>.Endpoints`.
             *
             * It is valid to create an object adapter with an empty string as its name. Such an object adapter is
             * accessible via bidirectional connections. Attempts to create a named object adapter for which no
             * configuration can be found will raise an `InitializationException`.
             *
             * @param name - The object adapter name.
             * @returns A promise that resolves to the created object adapter.
             * @throws {@link InitializationException} - Thrown if the object adapter cannot be created.
             *
             * @see {@link createObjectAdapterWithEndpoints}
             * @see {@link ObjectAdapter}
             * @see {@link Properties}
             */
            createObjectAdapter(name: string): Promise<Ice.ObjectAdapter>;

            /**
             * Creates a new object adapter with the given endpoints.
             *
             * This operation sets the property `<name>.Endpoints`, and then calls {@link Communicator#createObjectAdapter}.
             * If the `name` parameter is an empty string, a UUID will be generated and used as the object adapter name.
             *
             * @param name - The object adapter name.
             * @param endpoints - The endpoints for the object adapter.
             * @returns A promise that resolves to the created object adapter.
             *
             * @see {@link createObjectAdapter}
             * @see {@link ObjectAdapter}
             * @see {@link Properties}
             */
            createObjectAdapterWithEndpoints(name: string, endpoints: string): Promise<Ice.ObjectAdapter>;

            /**
             * Creates a new object adapter with a router.
             *
             * This operation creates a routed object adapter. If the `name` parameter is an empty string, a UUID will be
             * generated and used as the object adapter name.
             *
             * @param name - The object adapter name.
             * @param router - The router to associate with the object adapter.
             * @returns A promise that resolves to the created object adapter.
             *
             * @see {@link createObjectAdapter}
             * @see {@link ObjectAdapter}
             * @see {@link Properties}
             */
            createObjectAdapterWithRouter(name: string, router: RouterPrx): Promise<Ice.ObjectAdapter>;

            /**
             * Retrieves the implicit context associated with this communicator.
             *
             * @returns The implicit context associated with this communicator, or `null` if the `Ice.ImplicitContext`
             *          property is not set or is set to `None`.
             */
            getImplicitContext(): Ice.ImplicitContext;

            /**
             * Retrieves the properties associated with this communicator.
             *
             * @returns The properties of the communicator.
             */
            getProperties(): Ice.Properties;

            /**
             * Retrieves the logger associated with this communicator.
             *
             * @returns The logger of the communicator.
             */
            getLogger(): Ice.Logger;

            /**
             * Retrieves the default router for this communicator.
             *
             * @returns The default router of the communicator.
             *
             * @see {@link setDefaultRouter}
             */
            getDefaultRouter(): RouterPrx;

            /**
             * Sets the communicator's default router.
             *
             * All newly created proxies will use this default router. To disable the default router, pass `null`.
             * Note that this operation has no effect on existing proxies.
             *
             * You can also set a router for an individual proxy by calling {@link ObjectPrx#ice_router} on the proxy.
             *
             * @param router - The default router to use for this communicator, or `null` to disable the default
             *                 router.
             *
             * @see {@link getDefaultRouter}
             * @see {@link createObjectAdapterWithRouter}
             */
            setDefaultRouter(router: RouterPrx | null): void;

            /**
             * Retrieves the communicator's default locator.
             *
             * @returns The default locator of the communicator.
             *
             * @see {@link setDefaultLocator}
             */
            getDefaultLocator(): LocatorPrx;

            /**
             * Sets the communicator's default locator.
             *
             * All newly created proxies will use this default locator. To disable the default locator, pass `null`.
             * Note that this operation has no effect on existing proxies.
             *
             * You can also set a locator for an individual proxy by calling {@link ObjectPrx#ice_locator} on the proxy.
             *
             * @param locator - The default locator to use for this communicator, or `null` to disable the default
             *                  locator.
             *
             * @see {@link getDefaultLocator}
             */
            setDefaultLocator(locator: LocatorPrx | null): void;

            /**
             * Retrieves the communicator's value factory manager.
             *
             * @returns The value factory manager of the communicator.
             *
             * @see {@link ValueFactoryManager}
             */
            getValueFactoryManager(): Ice.ValueFactoryManager;

            /**
             * Flushes any pending batch requests for this communicator.
             *
             * This operation flushes all batch requests invoked on fixed proxies for all connections associated with the
             * communicator. Any errors that occur while flushing a connection are ignored.
             *
             * @returns A promise that resolves when the flush operation is complete.
             */
            flushBatchRequests(): Promise<void>;
        }
    }
}
