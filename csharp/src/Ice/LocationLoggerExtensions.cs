// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Security.Cryptography.X509Certificates;
using Microsoft.Extensions.Logging;

namespace ZeroC.Ice
{
    internal static partial class LocationLoggerExtensions
    {
        private static readonly Action<ILogger, string, Protocol, IReadOnlyList<Endpoint>, Exception> _clearLocationEndpoints = LoggerMessage.Define<string, Protocol, IReadOnlyList<Endpoint>>(
            LogLevel.Trace,
            GetEventId(LocationEvent.ClearLocationEndpoints),
            "removed endpoints for location from locator cache location = {Location} protocol = {Protocol} endpoints = {Endpoints}");

        private static readonly Action<ILogger, Reference, IReadOnlyList<Endpoint>, Exception> _clearWellKnownProxyEndpoints = LoggerMessage.Define<Reference, IReadOnlyList<Endpoint>>(
            LogLevel.Trace,
            GetEventId(LocationEvent.ClearWellKnownProxyEndpoints),
            "removed well-known proxy with endpoints from locator cache well-known proxy = {Proxy}, endpoints {Endpoints}");

        private static readonly Action<ILogger, Reference, string, Exception> _clearWellKnownProxyWithoutEndpoints = LoggerMessage.Define<Reference, string>(
            LogLevel.Trace,
            GetEventId(LocationEvent.ClearWellKnownProxyWithoutEndpoints),
            "removed well-known proxy without endpoints from locator cache proxy = {Proxy}, location = {Location}");

        private static readonly Action<ILogger, string, Exception> _couldNotFindEndpointsForLocation = LoggerMessage.Define<string>(
            LogLevel.Debug,
            GetEventId(LocationEvent.CouldNotFindEndpointsForLocation),
            "could not find endpoint(s) for location = {Location}");

        private static readonly Action<ILogger, Reference, Exception> _couldNotFindEndpointsForWellKnownProxy = LoggerMessage.Define<Reference>(
            LogLevel.Debug,
            GetEventId(LocationEvent.CouldNotFindEndpointsForWellKnownProxy),
            "could not find endpoint(s) for well-known proxy = {Proxy}");

        private static readonly Action<ILogger, string, Protocol, IReadOnlyList<Endpoint>, Exception> _foundEntryForLocationInLocatorCache = LoggerMessage.Define<string, Protocol, IReadOnlyList<Endpoint>>(
            LogLevel.Trace,
            GetEventId(LocationEvent.FoundEntryForLocationInLocatorCache),
            "found entry for location in locator cache");

        private static readonly Action<ILogger, Reference, IReadOnlyList<Endpoint>, Exception> _foundEntryForWellKnownProxyInLocatorCache = LoggerMessage.Define<Reference, IReadOnlyList<Endpoint>>(
            LogLevel.Trace,
            GetEventId(LocationEvent.FoundEntryForWellKnownProxyInLocatorCache),
            "found entry for well-known proxy in locator cache well-known proxy = {Proxy}, endpoints = {Endpoints}");

        private static readonly Action<ILogger, Exception> _getLocatorRegistryFailure = LoggerMessage.Define(
            LogLevel.Error,
            GetEventId(LocationEvent.GetLocatorRegistryFailure),
            "failed to retrieve locator registry");

        private static readonly Action<ILogger, string, Reference, Exception> _invalidReferenceResolvingLocation = LoggerMessage.Define<string, Reference>(
            LogLevel.Debug,
            GetEventId(LocationEvent.InvalidReferenceResolvingLocation),
            "locator returned an invalid proxy when resolving location = {Location}, received = {Proxy}");

        private static readonly Action<ILogger, Reference, Reference, Exception> _invalidReferenceResolvingProxy = LoggerMessage.Define<Reference, Reference>(
            LogLevel.Debug,
            GetEventId(LocationEvent.InvalidReferenceResolvingProxy),
            "locator returned an invalid proxy when resolving proxy = {Proxy}, received = {Received}");

        private static readonly Action<ILogger, string, Exception> _registerProcessProxyFailure = LoggerMessage.Define<string>(
            LogLevel.Debug,
            GetEventId(LocationEvent.RegisterProcessProxyFailure),
            "could not register server `{ServerId}' with the locator registry");

        private static readonly Action<ILogger, string, Exception> _registerProcessProxySuccess = LoggerMessage.Define<string>(
            LogLevel.Debug,
            GetEventId(LocationEvent.RegisterProcessProxySuccess),
            "registered server `{ServerId}' with the locator registry");

        private static readonly Action<ILogger, string, Exception> _resolveLocationFailure = LoggerMessage.Define<string>(
            LogLevel.Debug,
            GetEventId(LocationEvent.ResolveLocationFailure),
            "failure resolving location {Location}");

        private static readonly Action<ILogger, Reference, Exception> _resolveWellKnownProxyEndpointsFailure = LoggerMessage.Define<Reference>(
            LogLevel.Debug,
            GetEventId(LocationEvent.ResolveWellKnownProxyEndpointsFailure),
            "failure resolving endpoints for well-known proxy {Proxy}");

        private static readonly Action<ILogger, string, Protocol, IReadOnlyList<Endpoint>, Exception> _resolvedLocation = LoggerMessage.Define<string, Protocol, IReadOnlyList<Endpoint>>(
            LogLevel.Debug,
            GetEventId(LocationEvent.ResolvedLocation),
            "resolved location using locator, adding to locator cache location = {Location}, protocol = {Protocol}, endpoints = {Endpoints}");

        private static readonly Action<ILogger, Reference, IReadOnlyList<Endpoint>, Exception> _resolvedWellKnownProxy = LoggerMessage.Define<Reference, IReadOnlyList<Endpoint>>(
            LogLevel.Debug,
            GetEventId(LocationEvent.ResolvedWellKnownProxy),
            "resolved well-known proxy using locator, adding to locator cache");

        private static readonly Action<ILogger, string, Exception> _resolvingLocation = LoggerMessage.Define<string>(
            LogLevel.Debug,
            GetEventId(LocationEvent.ResolvingLocation),
            "resolving location {Location}");

        private static readonly Action<ILogger, Reference, Exception> _resolvingWellKnownProxy = LoggerMessage.Define<Reference>(
            LogLevel.Debug,
            GetEventId(LocationEvent.ResolvingWellKnownProxy),
            "resolving well-known object {Proxy}");

        private static EventId GetEventId(LocationEvent e) => new EventId((int)e, e.ToString());

        internal static void LogClearLocationEndpoints(
            this ILogger logger,
            string location,
            Protocol protocol,
            IReadOnlyList<Endpoint> endpoints) =>
            _clearLocationEndpoints(logger, location, protocol, endpoints, null!);

        internal static void LogClearWellKnownProxyEndpoints(
            this ILogger logger,
            Reference reference,
            IReadOnlyList<Endpoint> endpoints) =>
            _clearWellKnownProxyEndpoints(logger, reference, endpoints, null!);

        internal static void LogClearWellKnownProxyWithoutEndpoints(
            this ILogger logger,
            Reference reference,
            IReadOnlyList<string> location) =>
            _clearWellKnownProxyWithoutEndpoints(logger, reference, location.ToLocationString(), null!);

        internal static void LogCouldNotFindEndpointsForLocation(
            this ILogger logger,
            IReadOnlyList<string> location) =>
            _couldNotFindEndpointsForLocation(logger, location.ToLocationString(), null!);

        internal static void LogCouldNotFindEndpointsForWellKnownProxy(this ILogger logger, Reference reference) =>
            _couldNotFindEndpointsForWellKnownProxy(logger, reference, null!);

        internal static void LogFoundEntryForLocationInLocatorCache(
            this ILogger logger,
            IReadOnlyList<string> location,
            Protocol protocol,
            IReadOnlyList<Endpoint> endpoints) =>
            _foundEntryForLocationInLocatorCache(logger, location.ToLocationString(), protocol, endpoints, null!);

        internal static void LogFoundEntryForWellKnownProxyInLocatorCache(
            this ILogger logger,
            Reference reference,
            IReadOnlyList<Endpoint> endpoints) =>
            _foundEntryForWellKnownProxyInLocatorCache(logger, reference, endpoints, null!);

        internal static void LogGetLocatorRegistryFailure(this ILogger logger, Exception ex) =>
            _getLocatorRegistryFailure(logger, ex);

        internal static void LogInvalidReferenceResolvingLocation(
            this ILogger logger,
            string location,
            Reference reference) =>
            _invalidReferenceResolvingLocation(logger, location, reference, null!);

        internal static void LogInvalidReferenceResolvingProxy(
            this ILogger logger,
            Reference proxy,
            Reference received) =>
            _invalidReferenceResolvingProxy(logger, proxy, received, null!);

        internal static void LogRegisterProcessProxyFailure(this ILogger logger, string serverId, Exception ex) =>
            _registerProcessProxyFailure(logger, serverId, ex);

        internal static void LogRegisterProcessProxySuccess(this ILogger logger, string serverId) =>
            _registerProcessProxySuccess(logger, serverId, null!);

        internal static void LogResolveLocationFailure(
            this ILogger logger,
            IReadOnlyList<string> location,
            Exception exception) =>
            _resolveLocationFailure(logger, location.ToLocationString(), exception);

        internal static void LogResolveWellKnownProxyEndpointsFailure(
            this ILogger logger,
            Reference reference,
            Exception exception) =>
            _resolveWellKnownProxyEndpointsFailure(logger, reference, exception);

        internal static void LogResolvedWellKnownProxy(
            this ILogger logger,
            Reference reference,
            IReadOnlyList<Endpoint> endpoints) =>
            _resolvedWellKnownProxy(logger, reference, endpoints, null!);

        internal static void LogResolvedLocation(
            this ILogger logger,
            IReadOnlyList<string> location,
            Protocol protocol,
            IReadOnlyList<Endpoint> endpoints) =>
            _resolvedLocation(logger, location.ToLocationString(), protocol, endpoints, null!);

        internal static void LogResolvingLocation(this ILogger logger, IReadOnlyList<string> location) =>
            _resolvingLocation(logger, location.ToLocationString(), null!);

        internal static void LogResolvingWellKnownProxy(this ILogger logger, Reference reference) =>
            _resolvingWellKnownProxy(logger, reference, null!);

        private enum LocationEvent
        {
            ClearLocationEndpoints = 10001,
            ClearWellKnownProxyEndpoints = 10002,
            ClearWellKnownProxyWithoutEndpoints = 10003,
            CouldNotFindEndpointsForLocation = 10004,
            CouldNotFindEndpointsForWellKnownProxy = 10005,
            FoundEntryForLocationInLocatorCache = 10006,
            FoundEntryForWellKnownProxyInLocatorCache = 10007,
            GetLocatorRegistryFailure = 10008,
            InvalidReferenceResolvingLocation = 10009,
            InvalidReferenceResolvingProxy = 10010,
            RegisterProcessProxyFailure = 10011,
            RegisterProcessProxySuccess = 10012,
            ResolveLocationFailure = 10013,
            ResolveWellKnownProxyEndpointsFailure = 10014,
            ResolvedLocation = 10015,
            ResolvedWellKnownProxy = 10016,
            ResolvingLocation = 10017,
            ResolvingWellKnownProxy = 10018,
        }
    }

    internal static class TlsLoggerExtensions
    {
        private static readonly Action<ILogger, X509ChainStatusFlags, Exception> _tlsCertificateChainError = LoggerMessage.Define<X509ChainStatusFlags>(
            LogLevel.Error,
            GetEventId(TlsEvent.TlsCertificateChainError),
            "Tls certificate chain error {Status}");

        private static readonly Action<ILogger, Exception> _tlsCertificateValidationFailed = LoggerMessage.Define(
            LogLevel.Error,
            GetEventId(TlsEvent.TlsCertificateValidationFailed),
            "Tls certificate validation failed {Status}");

        private static readonly Action<ILogger, string, Dictionary<string, string>, Exception> _tlsConnectionCreated = LoggerMessage.Define<string, Dictionary<string, string>>(
            LogLevel.Error,
            GetEventId(TlsEvent.TlsConnectionCreated),
            "Tls connection summary {Description} {TlsConnectionInfo}");

        private static readonly Action<ILogger, Exception> _tlsHostnameMismatch = LoggerMessage.Define(
            LogLevel.Error,
            GetEventId(TlsEvent.TlsHostnameMismatch),
            "Tls certificate validation failed - Hostname mismatch");

        private static readonly Action<ILogger, Exception> _tlsRemoteCertificateNotProvided = LoggerMessage.Define(
            LogLevel.Error,
            GetEventId(TlsEvent.TlsRemoteCertificateNotProvided),
            "Tls certificate validation failed - remote certificate not provided");

        private static readonly Action<ILogger, Exception> _tlsRemoteCertificateNotProvidedIgnored = LoggerMessage.Define(
            LogLevel.Debug,
            GetEventId(TlsEvent.TlsRemoteCertificateNotProvidedIgnored),
            "Tls certificate validation failed - remote certificate not provided (ignored)");

        internal static void LogTlsCertificateChainError(this ILogger logger, X509ChainStatusFlags status) =>
            _tlsCertificateChainError(logger, status, null!);

        internal static void LogTlsCertificateValidationFailed(this ILogger logger) =>
            _tlsCertificateValidationFailed(logger, null!);

        internal static void LogTlsConnectionCreated(this ILogger logger, string description, Dictionary<string, string> info) =>
            _tlsConnectionCreated(logger, description, info, null!);

        internal static void LogTlsHostnameMismatch(this ILogger logger) => _tlsHostnameMismatch(logger, null!);

        internal static void LogTlsRemoteCertificateNotProvided(this ILogger logger) => _tlsRemoteCertificateNotProvided(logger, null!);

        internal static void LogTlsRemoteCertificateNotProvidedIgnored(this ILogger logger) => _tlsRemoteCertificateNotProvidedIgnored(logger, null!);

        private static EventId GetEventId(TlsEvent e) => new EventId((int)e, e.ToString());

        private enum TlsEvent
        {
            TlsCertificateChainError,
            TlsCertificateValidationFailed,
            TlsConnectionCreated,
            TlsHostnameMismatch,
            TlsRemoteCertificateNotProvided,
            TlsRemoteCertificateNotProvidedIgnored
        }
    }


    internal static class TransportLoggerExtensions
    {
        private static readonly Action<ILogger, int, Exception> _receivedInvalidDatagram = LoggerMessage.Define<int>(
            LogLevel.Error,
            GetEventId(TransportEvent.ReceivedInvalidDatagram),
            "maximum datagram size of { buffer.Count } exceeded");
        private static readonly Action<ILogger, int, Exception> _receivedInvalidDatagram = LoggerMessage.Define<int>(
            LogLevel.Error,
            GetEventId(TransportEvent.ReceivedInvalidDatagram),
            "received datagram with {Bytes} bytes");

        internal static void LogReceivedInvalidDatagram(this ILogger logger, int bytes) =>
            _receivedInvalidDatagram(logger, bytes, null!);

        private static EventId GetEventId(TransportEvent e) => new EventId((int)e, e.ToString());

        private enum TransportEvent
        {
            MaxDatagramSizeExceed,
            ReceivedInvalidDatagram
        }
    }
}