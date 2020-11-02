// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using ZeroC.Ice.Instrumentation;

namespace ZeroC.Ice
{
    /// <summary>Factory function that creates a proxy from a reference.</summary>
    /// <typeparam name="T">The proxy type.</typeparam>
    /// <param name="reference">The underlying reference.</param>
    /// <returns>The new proxy.</returns>
    public delegate T ProxyFactory<T>(Reference reference) where T : IObjectPrx;

    /// <summary>Proxy provides extension methods for IObjectPrx.</summary>
    public static class Proxy
    {
        /// <summary>Tests whether this proxy points to a remote object derived from T. If so it returns a proxy of
        /// type T otherwise returns null.</summary>
        /// <param name="prx">The source proxy.</param>
        /// <param name="factory">The proxy factory used to manufacture the returned proxy.</param>
        /// <param name="context">The request context used for the remote
        /// <see cref="IObjectPrx.IceIsA(string, IReadOnlyDictionary{string, string}?, CancellationToken)"/>
        /// invocation.</param>
        /// <returns>A new proxy manufactured by the proxy factory (see factory parameter).</returns>
        public static T? CheckedCast<T>(
            this IObjectPrx prx,
            ProxyFactory<T> factory,
            IReadOnlyDictionary<string, string>? context = null) where T : class, IObjectPrx =>
            prx.IceIsA(typeof(T).GetIceTypeId()!, context) ? factory(prx.IceReference) : null;

        /// <summary>Creates a clone of this proxy, with a new identity and optionally other options. The clone
        /// is identical to this proxy except for its identity and other options set through parameters.</summary>
        /// <param name="prx">The source proxy.</param>
        /// <param name="factory">The proxy factory used to manufacture the clone.</param>
        /// <param name="cacheConnection">Determines whether or not the clone caches its connection (optional).</param>
        /// <param name="clearLocator">When set to true, the clone does not have an associated locator proxy (optional).
        /// </param>
        /// <param name="clearRouter">When set to true, the clone does not have an associated router proxy (optional).
        /// </param>
        /// <param name="connectionId">The connection ID of the clone (optional).</param>
        /// <param name="context">The context of the clone (optional).</param>
        /// <param name="encoding">The encoding of the clone (optional).</param>
        /// <param name="endpoints">The endpoints of the clone (optional).</param>
        /// <param name="facet">The facet of the clone (optional).</param>
        /// <param name="fixedConnection">The connection of the clone (optional). When specified, the clone is a fixed
        /// proxy. You can clone a non-fixed proxy into a fixed proxy but not vice-versa.</param>
        /// <param name="identity">The identity of the clone.</param>
        /// <param name="identityAndFacet">A relative URI string [category/]identity[#facet].</param>
        /// <param name="invocationMode">The invocation mode of the clone (optional). Applies only to ice1 proxies.
        /// </param>
        /// <param name="invocationTimeout">The invocation timeout of the clone (optional).</param>
        /// <param name="location">The location of the clone (optional).</param>
        /// <param name="locator">The locator proxy of the clone (optional).</param>
        /// <param name="locatorCacheTimeout">The locator cache timeout of the clone (optional).</param>
        /// <param name="oneway">Determines whether the clone is oneway or twoway (optional).</param>
        /// <param name="preferNonSecure">Determines whether the clone prefers non-secure connections over secure
        /// connections (optional).</param>
        /// <param name="router">The router proxy of the clone (optional).</param>
        /// <returns>A new proxy manufactured by the proxy factory (see factory parameter).</returns>
        public static T Clone<T>(
            this IObjectPrx prx,
            ProxyFactory<T> factory,
            bool? cacheConnection = null,
            bool clearLocator = false,
            bool clearRouter = false,
            string? connectionId = null,
            IReadOnlyDictionary<string, string>? context = null,
            Encoding? encoding = null,
            IEnumerable<Endpoint>? endpoints = null,
            string? facet = null,
            Connection? fixedConnection = null,
            Identity? identity = null,
            string? identityAndFacet = null,
            InvocationMode? invocationMode = null,
            TimeSpan? invocationTimeout = null,
            IEnumerable<string>? location = null,
            ILocatorPrx? locator = null,
            TimeSpan? locatorCacheTimeout = null,
            bool? oneway = null,
            bool? preferNonSecure = null,
            IRouterPrx? router = null) where T : class, IObjectPrx =>
            factory(prx.IceReference.Clone(cacheConnection,
                                           clearLocator,
                                           clearRouter,
                                           connectionId,
                                           context,
                                           encoding,
                                           endpoints,
                                           facet,
                                           fixedConnection,
                                           identity,
                                           identityAndFacet,
                                           invocationMode,
                                           invocationTimeout,
                                           location,
                                           locator,
                                           locatorCacheTimeout,
                                           oneway,
                                           preferNonSecure,
                                           router));

        /// <summary>Creates a clone of this proxy. The clone is identical to this proxy except for options set
        /// through parameters. This method returns this proxy instead of a new proxy in the event none of the options
        /// specified through the parameters change this proxy's options.</summary>
        /// <param name="prx">The source proxy.</param>
        /// <param name="cacheConnection">Determines whether or not the clone caches its connection (optional).</param>
        /// <param name="clearLocator">When set to true, the clone does not have an associated locator proxy (optional).
        /// </param>
        /// <param name="clearRouter">When set to true, the clone does not have an associated router proxy (optional).
        /// </param>
        /// <param name="connectionId">The connection ID of the clone (optional).</param>
        /// <param name="context">The context of the clone (optional).</param>
        /// <param name="encoding">The encoding of the clone (optional).</param>
        /// <param name="endpoints">The endpoints of the clone (optional).</param>
        /// <param name="fixedConnection">The connection of the clone (optional). When specified, the clone is a fixed
        /// proxy. You can clone a non-fixed proxy into a fixed proxy but not vice-versa.</param>
        /// <param name="invocationMode">The invocation mode of the clone (optional). Applies only to ice1 proxies.
        /// </param>
        /// <param name="invocationTimeout">The invocation timeout of the clone (optional).</param>
        /// <param name="location">The location of the clone (optional).</param>
        /// <param name="locator">The locator proxy of the clone (optional).</param>
        /// <param name="locatorCacheTimeout">The locator cache timeout of the clone (optional).</param>
        /// <param name="oneway">Determines whether the clone is oneway or twoway (optional).</param>
        /// <param name="preferNonSecure">Determines whether the clone prefers non-secure connections over secure
        /// connections (optional).</param>
        /// <param name="router">The router proxy of the clone (optional).</param>
        /// <returns>A new proxy with the same type as this proxy.</returns>
        public static T Clone<T>(
            this T prx,
            bool? cacheConnection = null,
            bool clearLocator = false,
            bool clearRouter = false,
            string? connectionId = null,
            IReadOnlyDictionary<string, string>? context = null,
            Encoding? encoding = null,
            IEnumerable<Endpoint>? endpoints = null,
            Connection? fixedConnection = null,
            InvocationMode? invocationMode = null,
            TimeSpan? invocationTimeout = null,
            IEnumerable<string>? location = null,
            ILocatorPrx? locator = null,
            TimeSpan? locatorCacheTimeout = null,
            bool? oneway = null,
            bool? preferNonSecure = null,
            IRouterPrx? router = null) where T : IObjectPrx
        {
            Reference clone = prx.IceReference.Clone(cacheConnection,
                                                     clearLocator,
                                                     clearRouter,
                                                     connectionId,
                                                     context,
                                                     encoding,
                                                     endpoints,
                                                     facet: null,
                                                     fixedConnection,
                                                     identity: null,
                                                     identityAndFacet: null,
                                                     invocationMode,
                                                     invocationTimeout,
                                                     location,
                                                     locator,
                                                     locatorCacheTimeout,
                                                     oneway,
                                                     preferNonSecure,
                                                     router);

            // Reference.Clone never returns a new reference == to itself.
            return ReferenceEquals(clone, prx.IceReference) ? prx : (T)prx.Clone(clone);
        }

        /// <summary>Returns the cached Connection for this proxy. If the proxy does not yet have an established
        /// connection, it does not attempt to create a connection.</summary>
        /// <returns>The cached Connection for this proxy (null if the proxy does not have
        /// an established connection).</returns>
        public static Connection? GetCachedConnection(this IObjectPrx prx) => prx.IceReference.GetCachedConnection();

        /// <summary>Returns the Connection for this proxy. If the proxy does not yet have an established connection,
        /// it first attempts to create a connection.</summary>
        /// <returns>The Connection for this proxy or null if colocation optimization is used.</returns>
        public static Connection GetConnection(this IObjectPrx prx)
        {
            try
            {
                ValueTask<Connection> task = prx.GetConnectionAsync(cancel: default);
                return task.IsCompleted ? task.Result : task.AsTask().Result;
            }
            catch (AggregateException ex)
            {
                Debug.Assert(ex.InnerException != null);
                throw ExceptionUtil.Throw(ex.InnerException);
            }
        }

        /// <summary>Returns the Connection for this proxy. If the proxy does not yet have an established connection,
        /// it first attempts to create a connection.</summary>
        /// <returns>The Connection for this proxy or null if colocation optimization is used.</returns>
        public static async ValueTask<Connection> GetConnectionAsync(
            this IObjectPrx prx,
            CancellationToken cancel = default)
        {
            Reference reference = prx.IceReference;
            Connection? connection = reference.GetActiveConnection();
            if (connection != null)
            {
                // We have an active connection return it
                return connection;
            }
            else
            {
                // Go throw the list of endpoints connectors in order and return the first connection we found or create.
                (IReadOnlyList<Endpoint>? endpoints, _) =
                    await reference.GetEndpointsAsync(cancel).ConfigureAwait(false);
                Debug.Assert(endpoints.Count > 0);

                TransportException? lastException = null;
                foreach (Endpoint endpoint in endpoints)
                {
                    try
                    {
                        IReadOnlyList<IConnector>? connectors =
                            await endpoint.GetConnectorsAsync(cancel).ConfigureAwait(false);
                        foreach (IConnector connector in connectors)
                        {
                            try
                            {
                                // Get the connection, this will eventually establish a connection if needed.
                                return await reference.GetOrCreateConnectionAsync(endpoint,
                                                                                  connector,
                                                                                  cancel).ConfigureAwait(false);
                            }
                            catch (TransportException ex)
                            {
                                lastException = ex;
                            }
                        }
                    }
                    catch (TransportException ex)
                    {
                        lastException = ex;
                    }
                }
                Debug.Assert(lastException != null);
                throw ExceptionUtil.Throw(lastException);
            }
         }

        /// <summary>Forwards an incoming request to another Ice object represented by the <paramref name="proxy"/>
        /// parameter.</summary>
        /// <remarks>When the incoming request frame's protocol and proxy's protocol are different, this method
        /// automatically bridges between these two protocols. When proxy's protocol is ice1, the resulting outgoing
        /// request frame is never compressed.</remarks>
        /// <param name="proxy">The proxy for the target Ice object.</param>
        /// <param name="oneway">When true, the request is sent as a oneway request. When false, it is sent as a
        /// two-way request.</param>
        /// <param name="request">The incoming request frame to forward to proxy's target.</param>
        /// <param name="progress">Sent progress provider.</param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        /// <returns>A task holding the response frame.</returns>
        public static async ValueTask<OutgoingResponseFrame> ForwardAsync(
            this IObjectPrx proxy,
            bool oneway,
            IncomingRequestFrame request,
            IProgress<bool>? progress = null,
            CancellationToken cancel = default)
        {
            var forwardedRequest = new OutgoingRequestFrame(proxy, request, cancel: cancel);
            IncomingResponseFrame response =
                await proxy.InvokeAsync(forwardedRequest, oneway, progress).ConfigureAwait(false);
            return new OutgoingResponseFrame(request, response);
        }

        public static Task<IncomingResponseFrame> InvokeAsync(
            this IObjectPrx proxy,
            OutgoingRequestFrame request,
            bool oneway = false,
            IProgress<bool>? progress = null)
        {
            switch (proxy.InvocationMode)
            {
                case InvocationMode.BatchOneway:
                case InvocationMode.BatchDatagram:
                    Debug.Assert(false); // not implemented
                    return default;
                case InvocationMode.Datagram when !oneway:
                    throw new InvalidOperationException("cannot make two-way call on a datagram proxy");
                default:
                    return InvokeWithInterceptorsAsync(proxy,
                                                       request,
                                                       oneway,
                                                       0,
                                                       progress,
                                                       request.CancellationToken);
            }

            Task<IncomingResponseFrame> InvokeWithInterceptorsAsync(
                IObjectPrx proxy,
                OutgoingRequestFrame request,
                bool oneway,
                int i,
                IProgress<bool>? progress,
                CancellationToken cancel)
            {
                cancel.ThrowIfCancellationRequested();
                if (i < proxy.Communicator.InvocationInterceptors.Count)
                {
                    // Call the next interceptor in the chain
                    InvocationInterceptor interceptor = proxy.Communicator.InvocationInterceptors[i++];
                    return interceptor(
                        proxy,
                        request,
                        (target, request, cancel) =>
                            InvokeWithInterceptorsAsync(target, request, oneway, i, progress, cancel),
                        cancel);
                }
                else
                {
                    // After we went down the interceptor chain make the invocation.
                    return PerformInvokeAsync(request, oneway, progress, cancel);
                }
            }

            async Task<IncomingResponseFrame> PerformInvokeAsync(
                OutgoingRequestFrame request,
                bool oneway,
                IProgress<bool>? progress,
                CancellationToken cancel)
            {
                request.Finish();
                Reference reference = proxy.IceReference;

                IInvocationObserver? observer = ObserverHelper.GetInvocationObserver(proxy,
                                                                                     request.Operation,
                                                                                     request.Context);
                int attempt = 1;
                // If the request size is greater than Ice.RetryRequestSizeMax or the size of the request
                // would increase the buffer retry size beyond Ice.RetryBufferSizeMax we release the request
                // after it was sent to avoid holding too much memory and we wont retry in case of a failure.
                int requestSize = request.Size;
                bool releaseRequestAfterSent =
                    requestSize > reference.Communicator.RetryRequestSizeMax ||
                    !reference.Communicator.IncRetryBufferSize(requestSize);
                bool clearedLocatorCache = false;

                try
                {
                    IncomingResponseFrame? response = null;
                    Exception? lastException = null;

                    int firstEndpoint = 0;
                    int nextEndpoint = 0;
                    IReadOnlyList<Endpoint>? endpoints = null;
                    bool cachedEndpoints = false;

                    IReadOnlyList<IConnector>? connectors = null;
                    List<IConnector>? excludedConnectors = null;

                    IConnector? connector = null;
                    Endpoint? endpoint = null;
                    while (true)
                    {
                        bool sent = false;
                        RetryPolicy retryPolicy = RetryPolicy.NoRetry;
                        IChildInvocationObserver? childObserver = null;
                        Connection? connection = null;
                        try
                        {
                            connection = reference.GetActiveConnection();
                            if (connection == null)
                            {
                                if (endpoints == null)
                                {
                                    // Get the reference endpoints, endpoints associated with recent transport failures are
                                    // always last, throw NoEndpointException if it cannot obtain an endpoint.
                                    (endpoints, cachedEndpoints) = await reference.GetEndpointsAsync(cancel).ConfigureAwait(false);
                                    nextEndpoint = 0;
                                }
                                firstEndpoint = nextEndpoint;

                                Debug.Assert(endpoints.Count > 0);
                                do
                                {
                                    endpoint = endpoints[nextEndpoint++];
                                    if (nextEndpoint == endpoints.Count)
                                    {
                                        nextEndpoint = 0;
                                    }

                                    // Get the connectors for the next endpoint, connectors associated with recent
                                    // transport failures are always last, throw DNSException if failed to resolve the
                                    // endpoint host.
                                    try
                                    {
                                        connectors = await endpoint.GetConnectorsAsync(cancel).ConfigureAwait(false);
                                        if (excludedConnectors != null)
                                        {
                                            connectors = connectors.Except(excludedConnectors).ToImmutableList();
                                        }
                                        if (connectors.Count == 0)
                                        {
                                            throw new NoEndpointException();
                                        }

                                        for (int i = 0; i < connectors.Count;)
                                        {
                                            try
                                            {
                                                // Get a connection that use the given endpoint and connector or create a
                                                // new one if there is no active connections for the given endpoint
                                                // connector pair.
                                                connection = await reference.GetOrCreateConnectionAsync(endpoint,
                                                                                                        connectors[i],
                                                                                                        cancel).ConfigureAwait(false);
                                                break;
                                            }
                                            catch
                                            {
                                                reference.Communicator.OutgoingConnectionFactory.AddTransportFailure(endpoint, connectors[i]);
                                                if (++i == connectors.Count)
                                                {
                                                    throw; // No more endpoints to try
                                                }
                                            }
                                        }
                                    }
                                    catch
                                    {
                                        reference.Communicator.OutgoingConnectionFactory.AddTransportFailure(endpoint, null);
                                        if (nextEndpoint == firstEndpoint)
                                        {
                                            // We try all endpoint and connectors and didn't succeed getting or
                                            // creating a new connection, if this is a a indirect reference and
                                            // we are using cached endpoints try again with fresh endpoints.
                                            if (reference.IsIndirect && cachedEndpoints && !clearedLocatorCache)
                                            {
                                                clearedLocatorCache = true;
                                                reference.LocatorInfo!.ClearCache(reference);
                                                (endpoints, cachedEndpoints) = await reference.GetEndpointsAsync(cancel).ConfigureAwait(false);
                                                nextEndpoint = 0;
                                            }
                                            else
                                            {
                                                throw;
                                            }
                                        }
                                    }
                                }
                                while (connection == null);
                            }
                            connector ??= connection.Connector;

                            cancel.ThrowIfCancellationRequested();

                            // Create the outgoing stream.
                            using TransceiverStream stream = connection.CreateStream(!oneway);

                            childObserver = observer?.GetChildInvocationObserver(connection, request.Size);
                            childObserver?.Attach();

                            // TODO: support for streaming data, fin should be false if there's data to stream.
                            bool fin = true;

                            // Send the request and wait for the sending to complete.
                            await stream.SendRequestFrameAsync(request, fin, cancel).ConfigureAwait(false);

                            // The request is sent, notify the progress callback.
                            // TODO: Get rid of the sentSynchronously parameter which is always false now?
                            if (progress != null)
                            {
                                progress.Report(false);
                                progress = null; // Only call the progress callback once (TODO: revisit this?)
                            }
                            if (releaseRequestAfterSent)
                            {
                                // TODO release the request
                            }
                            sent = true;
                            lastException = null;

                            if (oneway)
                            {
                                return IncomingResponseFrame.WithVoidReturnValue(request.Protocol, request.Encoding);
                            }

                            // TODO: the synchronous boolean is no longer used. It was used to allow the reception
                            // of the response frame to be ran synchronously from the IO thread. Supporting this
                            // might still be possible depending on the underlying transport but it would be quite
                            // complex. So get rid of the synchronous boolean and simplify the proxy generated code?

                            // Wait for the reception of the response.
                            (response, fin) = await stream.ReceiveResponseFrameAsync(cancel).ConfigureAwait(false);

                            if (childObserver != null)
                            {
                                // Detach now to not count as a remote failure the 1.1 system exception which might
                                // be raised below.
                                childObserver.Reply(response.Size);
                                childObserver.Detach();
                                childObserver = null;
                            }

                            if (!fin)
                            {
                                // TODO: handle received stream data.
                            }

                            // If success, just return the response!
                            if (response.ResultType == ResultType.Success)
                            {
                                return response;
                            }

                            // Get the retry policy.
                            observer?.RemoteException();
                            if (response.Encoding == Encoding.V11)
                            {
                                retryPolicy = Ice1Definitions.GetRetryPolicy(response, reference);
                            }
                            else if (response.BinaryContext.TryGetValue((int)BinaryContext.RetryPolicy,
                                                                        out ReadOnlyMemory<byte> value))
                            {
                                retryPolicy = value.Read(istr => new RetryPolicy(istr));
                            }
                        }
                        catch (NoEndpointException ex)
                        {
                            // The reference has no endpoints or the previous retry policy asked to retry on a
                            // different replica but no more replicas are available (in this case, we throw
                            // the previous exception instead of the NoEndpointException).
                            if (response == null && lastException == null)
                            {
                                lastException = ex;
                            }
                            childObserver?.Failed(ex.GetType().FullName ?? "System.Exception");
                        }
                        catch (TransportException ex)
                        {
                            var closedException = ex as ConnectionClosedException;
                            connector ??= ex.Connector;
                            if (connector != null && closedException == null)
                            {
                                reference.Communicator.OutgoingConnectionFactory.AddTransportFailure(endpoint, connector);
                            }

                            lastException = ex;
                            childObserver?.Failed(ex.GetType().FullName ?? "System.Exception");

                            // Retry transport exceptions if the request is idempotent, was not sent or if the
                            // connection was gracefully closed by the peer (in which case it's safe to retry).
                            if ((closedException?.IsClosedByPeer ?? false) || request.IsIdempotent || !sent)
                            {
                                retryPolicy = ex.RetryPolicy;
                            }
                        }
                        catch (Exception ex)
                        {
                            lastException = ex;
                            childObserver?.Failed(ex.GetType().FullName ?? "System.Exception");
                        }
                        finally
                        {
                            childObserver?.Detach();
                        }

                        if (sent && releaseRequestAfterSent)
                        {
                            if (reference.Communicator.TraceLevels.Retry >= 1)
                            {
                                TraceRetry("request failed with retryable exception but the request is not retryable " +
                                           "because\n" + (requestSize > reference.Communicator.RetryRequestSizeMax ?
                                           "the request size exceeds Ice.RetryRequestSizeMax, " :
                                           "the retry buffer size would exceed Ice.RetryBufferSizeMax, ") +
                                           "passing exception through to the application",
                                           attempt,
                                           retryPolicy,
                                           lastException);
                            }
                            break; // We cannot retry, get out of the loop
                        }
                        else if (retryPolicy == RetryPolicy.NoRetry)
                        {
                            break; // We cannot retry, get out of the loop
                        }
                        else if (++attempt > reference.Communicator.RetryMaxAttempts)
                        {
                            if (reference.Communicator.TraceLevels.Retry >= 1)
                            {
                                TraceRetry("request failed with retryable exception but it was the final attempt,\n" +
                                           "passing exception through to the application",
                                           attempt,
                                           retryPolicy,
                                           lastException);
                            }
                            break; // We cannot retry, get out of the loop
                        }
                        else
                        {
                            // Retry
                            Debug.Assert(attempt <= reference.Communicator.RetryMaxAttempts &&
                                         retryPolicy != RetryPolicy.NoRetry,
                                         $"attempt: {attempt} retry-policy: {retryPolicy}");
                            if (retryPolicy == RetryPolicy.OtherReplica)
                            {
                                Debug.Assert(connector != null);
                                excludedConnectors ??= new List<IConnector>();
                                excludedConnectors.Add(connector);
                                if (reference.Communicator.TraceLevels.Retry >= 1)
                                {
                                    reference.Communicator.Logger.Trace(TraceLevels.RetryCategory,
                                                                        $"excluding connector\n{connector}");
                                }
                            }

                            if (reference.IsConnectionCached && connection != null)
                            {
                                reference.ClearConnection(connection);
                            }

                            // If we get a remote exception using an indirect reference clear the locator cache once
                            // per retry sequence to ensure we are not using any stale endpoints.
                            if (reference.IsIndirect && !clearedLocatorCache && response != null)
                            {
                                reference.LocatorInfo?.ClearCache(reference);
                                clearedLocatorCache = true;
                                endpoints = null;
                            }
                            else if (endpoints != null && ++nextEndpoint == endpoints.Count)
                            {
                                // If we tried all endpoints we start over
                                nextEndpoint = 0;
                            }

                            if (reference.Communicator.TraceLevels.Retry >= 1)
                            {
                                TraceRetry("retrying request because of retryable exception",
                                           attempt,
                                           retryPolicy,
                                           lastException);
                            }

                            if (retryPolicy.Retryable == Retryable.AfterDelay && retryPolicy.Delay != TimeSpan.Zero)
                            {
                                // The delay task can be canceled either by the user code using the provided
                                // cancellation token or if the communicator is destroyed.
                                using var tokenSource = CancellationTokenSource.CreateLinkedTokenSource(
                                    cancel,
                                    proxy.Communicator.CancellationToken);
                                await Task.Delay(retryPolicy.Delay, tokenSource.Token).ConfigureAwait(false);
                            }

                            observer?.Retried();
                        }
                    }

                    // No more retries or can't retry, throw the exception and return the remote exception
                    if (lastException != null)
                    {
                        observer?.Failed(lastException.GetType().FullName ?? "System.Exception");
                        throw ExceptionUtil.Throw(lastException);
                    }
                    else
                    {
                        observer?.Failed("System.Exception"); // TODO cleanup observer logic
                        Debug.Assert(response != null && response.ResultType == ResultType.Failure);
                        return response;
                    }
                }
                finally
                {
                    if (!releaseRequestAfterSent)
                    {
                        reference.Communicator.DecRetryBufferSize(requestSize);
                    }
                    // TODO release the request memory if not already done after sent.
                    // TODO: Use IDisposable for observers, this will allow using "using".
                    observer?.Detach();
                }
            }

            void TraceRetry(string message, int attempt, RetryPolicy policy, Exception? exception = null)
            {
                var sb = new StringBuilder();
                sb.Append(message);
                sb.Append("\nproxy = ");
                sb.Append(proxy);
                sb.Append("\noperation = ");
                sb.Append(request.Operation);
                if (attempt <= proxy.Communicator.RetryMaxAttempts)
                {
                    sb.Append("\nrequest attempt = ");
                    sb.Append(attempt);
                    sb.Append('/');
                    sb.Append(proxy.Communicator.RetryMaxAttempts);
                }
                sb.Append("\nretry policy = ");
                sb.Append(policy);
                if (exception != null)
                {
                    sb.Append("\nexception = ");
                    sb.Append(exception);
                }
                else
                {
                    sb.Append("\nexception = remote exception");
                }
                proxy.Communicator.Logger.Trace(TraceLevels.RetryCategory, sb.ToString());
            }
        }

        /// <summary>Produces a string representation of a location.</summary>
        /// <param name="location">The location.</param>
        /// <returns>The location as a percent-escaped string with segments separated by '/'.</returns>
        public static string ToLocationString(this IEnumerable<string> location) =>
            string.Join('/', location.Select(s => Uri.EscapeDataString(s)));
    }
}
