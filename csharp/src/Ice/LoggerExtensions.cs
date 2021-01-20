// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using Microsoft.Extensions.Logging;

namespace ZeroC.Ice
{
    internal static class LocatorLoggerExtensions
    {
        private static readonly Action<ILogger, Exception> _getLocatorRegistryFailure = LoggerMessage.Define(
            LogLevel.Error,
            new EventId(1000, nameof(TraceGetLocatorRegistryFailure)),
            "failed to retrieve locator registry:");

        private static readonly Action<ILogger, string, Exception> _setServerProcessProxyFailure = LoggerMessage.Define<string>(
            LogLevel.Error,
            new EventId(1001, nameof(TraceGetLocatorRegistryFailure)),
            "could not register server `{ServerId}' with the locator registry:");

        private static readonly Action<ILogger, string, Exception> _setServerProcessProxy = LoggerMessage.Define<string>(
            LogLevel.Information,
            new EventId(1002, nameof(TraceSetServerProcessProxy)),
            "registered server `{serverId}' with the locator registry");

        internal static void TraceGetLocatorRegistryFailure(this ILogger logger, Exception ex) =>
            _getLocatorRegistryFailure(logger, ex);

        internal static void TraceSetServerProcessProxyFailure(this ILogger logger, string serverId, Exception ex) =>
            _setServerProcessProxyFailure(logger, serverId, ex);

        internal static void TraceSetServerProcessProxy(this ILogger logger, string serverId) =>
            _setServerProcessProxy(logger, serverId, null!);
    }
}