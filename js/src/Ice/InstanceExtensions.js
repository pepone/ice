//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Instance, StateDestroyInProgress, StateDestroyed } from "./Instance.js";
import { AsyncResultBase } from "./AsyncResultBase.js";
import { DefaultsAndOverrides } from "./DefaultsAndOverrides.js";
import { EndpointFactoryManager } from "./EndpointFactoryManager.js";
import { ImplicitContext } from "./ImplicitContext.js";
import { LocatorManager } from "./LocatorManager.js";
import { ObjectAdapterFactory } from "./ObjectAdapterFactory.js";
import { OutgoingConnectionFactory } from "./OutgoingConnectionFactory.js";
import { Properties } from "./Properties.js";
import { ReferenceFactory } from "./ReferenceFactory.js";
import { RetryQueue } from "./RetryQueue.js";
import { RouterManager } from "./RouterManager.js";
import { Timer } from "./Timer.js";
import { TraceLevels } from "./TraceLevels.js";
import { ValueFactoryManager } from "./ValueFactoryManager.js";
import { LocalException } from "./Exception.js";
import { CommunicatorDestroyedException, InitializationException } from "./LocalException.js";
import { getProcessLogger } from "./ProcessLogger.js";
import { ToStringMode } from "./ToStringMode.js";
import { ProtocolInstance } from "./ProtocolInstance.js";
import { TcpEndpointFactory } from "./TcpEndpointFactory.js";
import { WSEndpointFactory } from "./WSEndpointFactory.js";
import { Promise } from "./Promise.js";
import { ConnectionOptions } from "./ConnectionOptions.js";
import { StringUtil } from "./StringUtil.js";

import { Ice as Ice_Router } from "./Router.js";
const { RouterPrx } = Ice_Router;
import { Ice as Ice_Locator } from "./Locator.js";
const { LocatorPrx } = Ice_Locator;

import { Ice as Ice_EndpointTypes } from "./EndpointTypes.js";
const { TCPEndpointType, WSEndpointType, SSLEndpointType, WSSEndpointType } = Ice_EndpointTypes;

import { Debug } from "./Debug.js";

let _oneOfDone = undefined;
let _printStackTraces = false;

Instance.prototype.initializationData = function () {
    //
    // No check for destruction. It must be possible to access the
    // initialization data after destruction.
    //
    // This value is immutable.
    //
    return this._initData;
};

Instance.prototype.traceLevels = function () {
    // This value is immutable.
    Debug.assert(this._traceLevels !== null);
    return this._traceLevels;
};

Instance.prototype.defaultsAndOverrides = function () {
    // This value is immutable.
    Debug.assert(this._defaultsAndOverrides !== null);
    return this._defaultsAndOverrides;
};

Instance.prototype.routerManager = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._routerManager !== null);
    return this._routerManager;
};

Instance.prototype.locatorManager = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._locatorManager !== null);
    return this._locatorManager;
};

Instance.prototype.referenceFactory = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._referenceFactory !== null);
    return this._referenceFactory;
};

Instance.prototype.outgoingConnectionFactory = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._outgoingConnectionFactory !== null);
    return this._outgoingConnectionFactory;
};

Instance.prototype.objectAdapterFactory = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._objectAdapterFactory !== null);
    return this._objectAdapterFactory;
};

Instance.prototype.retryQueue = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._retryQueue !== null);
    return this._retryQueue;
};

Instance.prototype.timer = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._timer !== null);
    return this._timer;
};

Instance.prototype.endpointFactoryManager = function () {
    if (this._state === StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    Debug.assert(this._endpointFactoryManager !== null);
    return this._endpointFactoryManager;
};

Instance.prototype.messageSizeMax = function () {
    // This value is immutable.
    return this._messageSizeMax;
};

Instance.prototype.batchAutoFlushSize = function () {
    // This value is immutable.
    return this._batchAutoFlushSize;
};

Instance.prototype.classGraphDepthMax = function () {
    // This value is immutable.
    return this._classGraphDepthMax;
};

Instance.prototype.toStringMode = function () {
    // this value is immutable
    return this._toStringMode;
};

Instance.prototype.getImplicitContext = function () {
    return this._implicitContext;
};

Instance.prototype.setDefaultLocator = function (locator) {
    if (this._state == StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    this._referenceFactory = this._referenceFactory.setDefaultLocator(locator);
};

Instance.prototype.setDefaultRouter = function (router) {
    if (this._state == StateDestroyed) {
        throw new CommunicatorDestroyedException();
    }

    this._referenceFactory = this._referenceFactory.setDefaultRouter(router);
};

Instance.prototype.setLogger = function (logger) {
    this._initData.logger = logger;
};

Instance.prototype.finishSetup = function (communicator, promise) {
    //
    // If promise == null, it means the caller is requesting a synchronous setup.
    // Otherwise, we resolve the promise after all initialization is complete.
    //
    try {
        if (this._initData.properties === null) {
            this._initData.properties = Properties.createProperties();
        }

        this._clientConnectionOptions = new ConnectionOptions(
            this._initData.properties.getIcePropertyAsInt("Ice.Connection.ConnectTimeout"),
            this._initData.properties.getIcePropertyAsInt("Ice.Connection.CloseTimeout"),
            this._initData.properties.getIcePropertyAsInt("Ice.Connection.IdleTimeout"),
            this._initData.properties.getIcePropertyAsInt("Ice.Connection.EnableIdleCheck") > 0,
            this._initData.properties.getIcePropertyAsInt("Ice.Connection.InactivityTimeout"),
        );

        if (_oneOfDone === undefined) {
            _printStackTraces = this._initData.properties.getPropertyAsIntWithDefault("Ice.PrintStackTraces", 0) > 0;

            _oneOfDone = true;
        }

        if (this._initData.logger === null) {
            this._initData.logger = getProcessLogger();
        }

        this._traceLevels = new TraceLevels(this._initData.properties);

        this._defaultsAndOverrides = new DefaultsAndOverrides(this._initData.properties, this._initData.logger);

        const defMessageSizeMax = 1024;
        let num = this._initData.properties.getPropertyAsIntWithDefault("Ice.MessageSizeMax", defMessageSizeMax);
        if (num < 1 || num > 0x7fffffff / 1024) {
            this._messageSizeMax = 0x7fffffff;
        } else {
            this._messageSizeMax = num * 1024; // Property is in kilobytes, _messageSizeMax in bytes
        }

        if (
            this._initData.properties.getProperty("Ice.BatchAutoFlushSize").length === 0 &&
            this._initData.properties.getProperty("Ice.BatchAutoFlush").length > 0
        ) {
            if (this._initData.properties.getPropertyAsInt("Ice.BatchAutoFlush") > 0) {
                this._batchAutoFlushSize = this._messageSizeMax;
            }
        } else {
            num = this._initData.properties.getPropertyAsIntWithDefault("Ice.BatchAutoFlushSize", 1024); // 1MB
            if (num < 1) {
                this._batchAutoFlushSize = num;
            } else if (num > 0x7fffffff / 1024) {
                this._batchAutoFlushSize = 0x7fffffff;
            } else {
                this._batchAutoFlushSize = num * 1024; // Property is in kilobytes, _batchAutoFlushSize in bytes
            }
        }

        num = this._initData.properties.getIcePropertyAsInt("Ice.ClassGraphDepthMax");
        if (num < 1 || num > 0x7fffffff) {
            this._classGraphDepthMax = 0x7fffffff;
        } else {
            this._classGraphDepthMax = num;
        }

        const toStringModeStr = this._initData.properties.getPropertyWithDefault("Ice.ToStringMode", "Unicode");
        if (toStringModeStr === "ASCII") {
            this._toStringMode = ToStringMode.ASCII;
        } else if (toStringModeStr === "Compat") {
            this._toStringMode = ToStringMode.Compat;
        } else if (toStringModeStr !== "Unicode") {
            throw new InitializationException("The value for Ice.ToStringMode must be Unicode, ASCII or Compat");
        }

        this._implicitContext = ImplicitContext.create(this._initData.properties.getProperty("Ice.ImplicitContext"));

        this._routerManager = new RouterManager();

        this._locatorManager = new LocatorManager(this._initData.properties);

        this._referenceFactory = new ReferenceFactory(this, communicator);

        this._endpointFactoryManager = new EndpointFactoryManager(this);

        const tcpInstance = new ProtocolInstance(this, TCPEndpointType, "tcp", false);
        const tcpEndpointFactory = new TcpEndpointFactory(tcpInstance);
        this._endpointFactoryManager.add(tcpEndpointFactory);

        const wsInstance = new ProtocolInstance(this, WSEndpointType, "ws", false);
        const wsEndpointFactory = new WSEndpointFactory(wsInstance, tcpEndpointFactory.clone(wsInstance));
        this._endpointFactoryManager.add(wsEndpointFactory);

        const sslInstance = new ProtocolInstance(this, SSLEndpointType, "ssl", true);
        const sslEndpointFactory = new TcpEndpointFactory(sslInstance);
        this._endpointFactoryManager.add(sslEndpointFactory);

        const wssInstance = new ProtocolInstance(this, WSSEndpointType, "wss", true);
        const wssEndpointFactory = new WSEndpointFactory(wssInstance, sslEndpointFactory.clone(wssInstance));
        this._endpointFactoryManager.add(wssEndpointFactory);

        this._outgoingConnectionFactory = new OutgoingConnectionFactory(communicator, this);

        if (this._initData.valueFactoryManager === null) {
            this._initData.valueFactoryManager = new ValueFactoryManager();
        }

        this._objectAdapterFactory = new ObjectAdapterFactory(this, communicator);

        this._retryQueue = new RetryQueue(this);
        const retryIntervals = this._initData.properties.getPropertyAsList("Ice.RetryIntervals");
        if (retryIntervals.length > 0) {
            this._retryIntervals = [];

            for (let i = 0; i < retryIntervals.length; i++) {
                let v;

                try {
                    v = StringUtil.toInt(retryIntervals[i]);
                } catch (ex) {
                    v = 0;
                }

                //
                // If -1 is the first value, no retry and wait intervals.
                //
                if (i === 0 && v === -1) {
                    break;
                }

                this._retryIntervals[i] = v > 0 ? v : 0;
            }
        } else {
            this._retryIntervals = [0];
        }

        this._timer = new Timer(this._initData.logger);

        const router = communicator.propertyToProxy("Ice.Default.Router");
        if (router !== null) {
            this._referenceFactory = this._referenceFactory.setDefaultRouter(new RouterPrx(router));
        }

        const loc = communicator.propertyToProxy("Ice.Default.Locator");
        if (loc !== null) {
            this._referenceFactory = this._referenceFactory.setDefaultLocator(new LocatorPrx(loc));
        }

        if (promise !== null) {
            promise.resolve(communicator);
        }
    } catch (ex) {
        if (promise !== null) {
            if (ex instanceof LocalException) {
                this.destroy().finally(() => promise.reject(ex));
            } else {
                promise.reject(ex);
            }
        } else {
            if (ex instanceof LocalException) {
                this.destroy();
            }
            throw ex;
        }
    }
};

Instance.prototype.destroy = function () {
    const promise = new AsyncResultBase(null, "destroy", null, this, null);

    //
    // If destroy is in progress, wait for it to be done. This is
    // necessary in case destroy() is called concurrently by
    // multiple threads.
    //
    if (this._state == StateDestroyInProgress) {
        if (!this._destroyPromises) {
            this._destroyPromises = [];
        }
        this._destroyPromises.push(promise);
        return promise;
    }
    this._state = StateDestroyInProgress;

    //
    // Shutdown and destroy all the incoming and outgoing Ice
    // connections and wait for the connections to be finished.
    //
    Promise.try(() => {
        if (this._objectAdapterFactory) {
            return this._objectAdapterFactory.shutdown();
        }
    })
        .then(() => {
            if (this._outgoingConnectionFactory !== null) {
                this._outgoingConnectionFactory.destroy();
            }

            if (this._objectAdapterFactory !== null) {
                return this._objectAdapterFactory.destroy();
            }
        })
        .then(() => {
            if (this._outgoingConnectionFactory !== null) {
                return this._outgoingConnectionFactory.waitUntilFinished();
            }
        })
        .then(() => {
            if (this._retryQueue) {
                this._retryQueue.destroy();
            }
            if (this._timer) {
                this._timer.destroy();
            }

            if (this._objectFactoryMap !== null) {
                this._objectFactoryMap.forEach(factory => factory.destroy());
                this._objectFactoryMap.clear();
            }

            if (this._routerManager) {
                this._routerManager.destroy();
            }
            if (this._locatorManager) {
                this._locatorManager.destroy();
            }
            if (this._endpointFactoryManager) {
                this._endpointFactoryManager.destroy();
            }

            if (this._initData.properties.getPropertyAsInt("Ice.Warn.UnusedProperties") > 0) {
                const unusedProperties = this._initData.properties.getUnusedProperties();
                if (unusedProperties.length > 0) {
                    const message = [];
                    message.push("The following properties were set but never read:");
                    unusedProperties.forEach(p => message.push("\n    ", p));
                    this._initData.logger.warning(message.join(""));
                }
            }

            this._objectAdapterFactory = null;
            this._outgoingConnectionFactory = null;
            this._retryQueue = null;
            this._timer = null;

            this._referenceFactory = null;
            this._routerManager = null;
            this._locatorManager = null;
            this._endpointFactoryManager = null;

            this._state = StateDestroyed;

            if (this._destroyPromises) {
                this._destroyPromises.forEach(p => p.resolve());
            }
            promise.resolve();
        })
        .catch(ex => {
            if (this._destroyPromises) {
                this._destroyPromises.forEach(p => p.reject(ex));
            }
            promise.reject(ex);
        });
    return promise;
};

Object.defineProperty(Instance.prototype, "clientConnectionOptions", {
    get: function () {
        return this._clientConnectionOptions;
    },
    enumerable: true,
});
