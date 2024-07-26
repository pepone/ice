//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

declare module "ice" {
    namespace Ice {
        /**
         * The base exception for the 3 NotExist exceptions.
         */
        class RequestFailedException extends LocalException {
            constructor(typeName: string, id: Identity, facet: string, operation: string);
            id: Identity;
            facet: string;
            operation: string;
        }

        /**
         * The dispatch could not find a servant for the identity carried by the request.
         */
        class ObjectNotExistException extends RequestFailedException {
            constructor(id?: Identity, facet?: string, operation?: string);
        }

        /**
         * The dispatch could not find a servant for the identity + facet carried by the request.
         */
        class FacetNotExistException extends RequestFailedException {
            constructor(id: Identity, facet: string, operation: string);
        }

        /**
         * The dispatch could not find the operation carried by the request on the target servant. This is typically due
         * to a mismatch in the Slice definitions, such as the client using Slice definitions newer than the server's.
         */
        class OperationNotExistException extends RequestFailedException {
            constructor(id: Identity, facet: string, operation: string);
        }

        /**
         * The dispatch failed with an exception that is not a {@link LocalException} or a {@link UserException}.
         */
        class UnknownException extends LocalException {
            /**
             * One-shot constructor to initialize all data members.
             * @param unknown This field is set to the textual representation of the unknown exception if available.
             * @param ice_cause The error that cause this exception.
             */
            constructor(message: string);
            unknown: string;
        }

        /**
         * The dispatch failed with a {@link LocalException} that is not one of the special marshal-able local exceptions.
         */
        class UnknownLocalException extends UnknownException {
            constructor(message: string);
        }

        /**
         * The dispatch returned a {@link UserException} that was not declared in the operation's exception specification.
         */
        class UnknownUserException extends UnknownException {
            constructor(message: string);
        }

        //
        // Protocol exceptions
        //

        /**
         * The base class for Ice protocol exceptions.
         */
        class ProtocolException extends LocalException {}

        /**
         * This exception indicates that the connection has been gracefully closed by the server.
         * The operation call that caused this exception has not been executed by the server. In most cases you will not get
         * this exception because the client will automatically retry the operation call in case the server shut down the
         * connection. However, if upon retry the server shuts down the connection again, and the retry limit has been reached,
         * then this exception is propagated to the application code.
         */
        class CloseConnectionException extends ProtocolException {
            constructor();
        }

        /**
         *  This exception reports an error during marshaling or unmarshaling.
         */
        class MarshalException extends ProtocolException {}

        //
        // Timeout exceptions
        //

        /**
         * This exception indicates a timeout condition.
         */
        class TimeoutException extends LocalException {}

        /**
         * This exception indicates a connection closure timeout condition.
         */
        class CloseTimeoutException extends TimeoutException {
            constructor();
        }

        /**
         * This exception indicates a connection establishment timeout condition.
         */
        class ConnectTimeoutException extends TimeoutException {
            constructor();
        }

        /**
         * This exception indicates that an invocation failed because it timed out.
         */
        class InvocationTimeoutException extends TimeoutException {
            constructor();
        }

        //
        // Syscall exceptions
        //

        /**
         * This exception is raised if a system error occurred in the server or client process.
         */
        class SyscallException extends LocalException {}

        //
        // Socket exceptions
        //

        /**
         * This exception indicates a socket error.
         */
        class SocketException extends SyscallException {}

        /**
         * This exception indicates a connection failure.
         */
        class ConnectFailedException extends SocketException {}

        /**
         * This exception indicates a lost connection.
         */
        class ConnectionLostException extends SocketException {}

        /**
         * This exception indicates a connection failure for which the server host actively refuses a connection.
         */
        class ConnectionRefusedException extends ConnectFailedException {}

        //
        // Other leaf local exceptions in alphabetical order.
        //

        /**
         * An attempt was made to register something more than once with the Ice run time.
         * This exception is raised if an attempt is made to register a servant, servant locator, facet, value factory,
         * plug-in, object adapter (etc.) more than once for the same ID.
         */
        class AlreadyRegisteredException extends LocalException {
            constructor(kindOfObject: string, id: string);
            kindOfObject: string;
            id: string;
        }

        /**
         * This exception is raised if the Communicator has been destroyed.
         */
        class CommunicatorDestroyedException extends LocalException {}

        /**
         * This exception indicates that a connection was closed forcefully.
         */
        class ConnectionAbortedException extends LocalException {
            constructor(message: string, closedByApplication: boolean);
            closedByApplication: boolean;
        }

        /**
         * This exception indicates that a connection was closed gracefully.
         */
        class ConnectionClosedException extends LocalException {
            constructor(message: string, closedByApplication: boolean);
            closedByApplication: boolean;
        }

        /**
         * This exception is raised if an unsupported feature is used.
         */
        class FeatureNotSupportedException extends LocalException {
            constructor(message: string);
        }

        /**
         * This exception indicates that an attempt has been made to change the connection properties of a fixed proxy.
         */
        class FixedProxyException extends LocalException {}

        /**
         * This exception is raised when a failure occurs during initialization.
         */
        class InitializationException extends LocalException {}

        /**
         * This exception indicates that an asynchronous invocation failed because it was canceled explicitly by the user.
         */
        class InvocationCanceledException extends LocalException {
            constructor();
        }

        /**
         * This exception is raised if no suitable endpoint is available.
         */
        class NoEndpointException extends LocalException {
            constructor(messageOrProxy: string | ObjectPrx);
        }

        /**
         * An attempt was made to find or deregister something that is not registered with the Ice run time or Ice locator.
         * This exception is raised if an attempt is made to remove a servant, servant locator, facet, value factory, plug-in,
         * object adapter (etc.) that is not currently registered. It's also raised if the Ice locator can't find an object or
         * object adapter when resolving an indirect proxy or when an object adapter is activated.
         */
        class NotRegisteredException extends LocalException {
            constructor(kindOfObject?: string, id?: string);
            kindOfObject: string;
            id: string;
        }

        /**
         * This exception is raised if an attempt is made to use a deactivated ObjectAdapter.
         */
        class ObjectAdapterDeactivatedException extends LocalException {
            constructor(name: string);
        }

        /**
         * This exception is raised if an ObjectAdapter cannot be activated.
         * This happens if the Locator detects another active ObjectAdapter with the same adapter ID.
         */
        class ObjectAdapterIdInUseException extends LocalException {
            constructor(adapterId: string);
        }

        /**
         * Reports a failure that occurred while parsing a string.
         */
        class ParseException extends LocalException {}

        /**
         * The operation can only be invoked with a two-way request.
         * This exception is raised if an attempt is made to invoke an operation with ice_oneway, ice_batchOneway,
         * ice_datagram, or ice_batchDatagram and the operation has a return value, out-parameters, or an exception
         * specification.
         */
        class TwowayOnlyException extends LocalException {
            constructor(operation: string);
        }
    }
}
