// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    public interface IConnectionManager
    {
        void Remove(Connection connection);
    }

    internal abstract class IncomingConnectionFactory : IAsyncDisposable
    {
        internal abstract Endpoint Endpoint { get; }

        public abstract ValueTask DisposeAsync();

        internal abstract void Activate();

        internal bool IsLocal(Endpoint endpoint) => endpoint.IsLocal(Endpoint);

        internal abstract void UpdateConnectionObservers();
    }

    // IncomingConnectionFactory for acceptor based transports.
    internal sealed class AcceptorIncomingConnectionFactory : IncomingConnectionFactory, IConnectionManager
    {
        internal override Endpoint Endpoint { get; }

        private readonly IAcceptor _acceptor;
        private Task? _acceptTask;
        private readonly ObjectAdapter _adapter;
        private readonly Communicator _communicator;
        private readonly HashSet<Connection> _connections = new();
        private bool _disposed;
        private readonly object _mutex = new();

        public override async ValueTask DisposeAsync()
        {
            if (_communicator.TraceLevels.Transport >= 1)
            {
                _communicator.Logger.Trace(TraceLevels.TransportCategory,
                    $"stopping to accept {Endpoint.TransportName} connections at {_acceptor}");
            }

            // Dispose of the acceptor and close the connections. It's important to perform this synchronously without
            // any await in between to guarantee that once Communicator.ShutdownAsync returns the communicator no
            // longer accepts any requests.

            lock (_mutex)
            {
                _disposed = true;
                _acceptor.Dispose();
            }

            // The connection set is immutable once _disposed = true
            var exception = new ObjectDisposedException($"{typeof(ObjectAdapter).FullName}:{_adapter.Name}");
            IEnumerable<Task> tasks = _connections.Select(connection => connection.GoAwayAsync(exception));

            // Wait for AcceptAsync and the connection closure to return.
            if (_acceptTask != null)
            {
                await _acceptTask.ConfigureAwait(false);
            }
            await Task.WhenAll(tasks).ConfigureAwait(false);
        }

        public void Remove(Connection connection)
        {
            lock (_mutex)
            {
                if (!_disposed)
                {
                    _connections.Remove(connection);
                }
            }
        }

        public override string ToString() => _acceptor.ToString()!;

        internal AcceptorIncomingConnectionFactory(ObjectAdapter adapter, Endpoint endpoint)
        {
            _communicator = adapter.Communicator;
            _adapter = adapter;
            _acceptor = endpoint.Acceptor(this, _adapter);
            Endpoint = _acceptor.Endpoint;

            if (_communicator.TraceLevels.Transport >= 1)
            {
                _communicator.Logger.Trace(TraceLevels.TransportCategory,
                    $"listening for {Endpoint.TransportName} connections\n{_acceptor.ToDetailedString()}");
            }
        }

        internal override void Activate()
        {
            if (_communicator.TraceLevels.Transport >= 1)
            {
                _communicator.Logger.Trace(TraceLevels.TransportCategory,
                    $"accepting {Endpoint.TransportName} connections at {_acceptor}");
            }

            // Start the asynchronous operation from the thread pool to prevent eventually accepting
            // synchronously new connections from this thread.
            lock (_mutex)
            {
                Debug.Assert(!_disposed);
                _acceptTask = Task.Factory.StartNew(AcceptAsync,
                                                    default,
                                                    TaskCreationOptions.None,
                                                    _adapter.TaskScheduler ?? TaskScheduler.Default);
            }
        }

        internal override void UpdateConnectionObservers()
        {
            lock (_mutex)
            {
                foreach (Connection connection in _connections)
                {
                    connection.UpdateObserver();
                }
            }
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage(
            "Reliability",
            "CA2007:Consider calling ConfigureAwait on the awaited task",
            Justification = "Ensure continuations execute on the object adapter scheduler if it is set")]
        private async ValueTask AcceptAsync()
        {
            while (true)
            {
                Connection? connection = null;
                try
                {
                    connection = await _acceptor.AcceptAsync();

                    if (_communicator.TraceLevels.Transport >= 2)
                    {
                        _communicator.Logger.Trace(TraceLevels.TransportCategory,
                            $"trying to accept {Endpoint.TransportName} connection\n{connection}");
                    }

                    lock (_mutex)
                    {
                        if (_disposed)
                        {
                            throw new ObjectDisposedException($"{typeof(ObjectAdapter).FullName}:{_adapter.Name}");
                        }

                        _connections.Add(connection);

                        // We don't wait for the connection to be activated. This could take a while for some transports
                        // such as TLS based transports where the handshake requires few round trips between the client
                        // and server.
                        _ = connection.InitializeAsync();
                    }
                }
                catch (Exception exception)
                {
                    if (connection != null)
                    {
                        await connection.GoAwayAsync(exception);
                    }
                    if (_disposed)
                    {
                        return;
                    }

                    // We print an error and wait for one second to avoid running in a tight loop in case the
                    // failures occurs immediately again. Failures here are unexpected and could be considered
                    // fatal.
                    _communicator.Logger.Error($"failed to accept connection:\n{exception}\n{_acceptor}");
                    await Task.Delay(TimeSpan.FromSeconds(1));
                    continue;
                }
            }
        }
    }

    // IncomingConnectionFactory for datagram based transports
    internal sealed class DatagramIncomingConnectionFactory : IncomingConnectionFactory
    {
        internal override Endpoint Endpoint { get; }

        private readonly Connection _connection;

        public override async ValueTask DisposeAsync()
        {
            var exception =
                new ObjectDisposedException($"{typeof(ObjectAdapter).FullName}:{_connection.Adapter!.Name}");
            await _connection.GoAwayAsync(exception).ConfigureAwait(false);
        }

        public override string ToString() => _connection.ToString()!;

        internal DatagramIncomingConnectionFactory(ObjectAdapter adapter, Endpoint endpoint)
        {
            _connection = endpoint.CreateDatagramServerConnection(adapter);
            Endpoint = _connection.Endpoint;
            _ = _connection.InitializeAsync();
        }

        internal override void Activate()
        {
        }

        internal override void UpdateConnectionObservers() => _connection.UpdateObserver();
    }
}
