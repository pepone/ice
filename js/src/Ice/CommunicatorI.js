//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Instance } from "./Instance";
import { AsyncResultBase } from "./AsyncResultBase";
import { Debug } from "./Debug";
import { CommunicatorDestroyedException } from "./LocalException";
import { identityToString, stringToIdentity } from "./IdentityUtil";
import { generateUUID } from "./UUID";

//
// Ice.Communicator
//
class Communicator
{
    constructor(initData)
    {
        this._instance = new Instance(initData);
    }

    //
    // Certain initialization tasks need to be completed after the
    // constructor.
    //
    finishSetup(promise)
    {
        this._instance.finishSetup(this, promise);
    }

    destroy()
    {
        return this._instance.destroy();
    }

    shutdown()
    {
        try
        {
            return this._instance.objectAdapterFactory().shutdown();
        }
        catch(ex)
        {
            Debug.assert(ex instanceof CommunicatorDestroyedException);
            return IcePromise.resolve();
        }
    }

    waitForShutdown()
    {
        try
        {
            return this._instance.objectAdapterFactory().waitForShutdown();
        }
        catch(ex)
        {
            Debug.assert(ex instanceof CommunicatorDestroyedException);
            return IcePromise.resolve();
        }
    }

    isShutdown()
    {
        try
        {
            return this._instance.objectAdapterFactory().isShutdown();
        }
        catch(ex)
        {
            if(!(ex instanceof CommunicatorDestroyedException))
            {
                throw ex;
            }
            return true;
        }
    }

    stringToProxy(s)
    {
        return this._instance.proxyFactory().stringToProxy(s);
    }

    proxyToString(proxy)
    {
        return this._instance.proxyFactory().proxyToString(proxy);
    }

    propertyToProxy(s)
    {
        return this._instance.proxyFactory().propertyToProxy(s);
    }

    proxyToProperty(proxy, prefix)
    {
        return this._instance.proxyFactory().proxyToProperty(proxy, prefix);
    }

    stringToIdentity(s)
    {
        return stringToIdentity(s);
    }

    identityToString(ident)
    {
        return identityToString(ident, this._instance.toStringMode());
    }

    createObjectAdapter(name)
    {
        const promise = new AsyncResultBase(this, "createObjectAdapter", this, null, null);
        this._instance.objectAdapterFactory().createObjectAdapter(name, null, promise);
        return promise;
    }

    createObjectAdapterWithEndpoints(name, endpoints)
    {
        if(name.length === 0)
        {
            name = generateUUID();
        }

        this.getProperties().setProperty(name + ".Endpoints", endpoints);
        const promise = new AsyncResultBase(this, "createObjectAdapterWithEndpoints", this, null, null);
        this._instance.objectAdapterFactory().createObjectAdapter(name, null, promise);
        return promise;
    }

    createObjectAdapterWithRouter(name, router)
    {
        if(name.length === 0)
        {
            name = generateUUID();
        }

        const promise = new AsyncResultBase(this, "createObjectAdapterWithRouter", this, null, null);

        //
        // We set the proxy properties here, although we still use the proxy supplied.
        //
        this.proxyToProperty(router, name + ".Router").forEach((value, key) =>
            {
                this.getProperties().setProperty(key, value);
            });

        this._instance.objectAdapterFactory().createObjectAdapter(name, router, promise);
        return promise;
    }

    addObjectFactory(factory, id)
    {
        this._instance.addObjectFactory(factory, id);
    }

    findObjectFactory(id)
    {
        return this._instance.findObjectFactory(id);
    }

    getValueFactoryManager()
    {
        return this._instance.initializationData().valueFactoryManager;
    }

    getImplicitContext()
    {
        return this._instance.getImplicitContext();
    }

    getProperties()
    {
        return this._instance.initializationData().properties;
    }

    getLogger()
    {
        return this._instance.initializationData().logger;
    }

    getDefaultRouter()
    {
        return this._instance.referenceFactory().getDefaultRouter();
    }

    setDefaultRouter(router)
    {
        this._instance.setDefaultRouter(router);
    }

    getDefaultLocator()
    {
        return this._instance.referenceFactory().getDefaultLocator();
    }

    setDefaultLocator(locator)
    {
        this._instance.setDefaultLocator(locator);
    }

    flushBatchRequests()
    {
        return this._instance.outgoingConnectionFactory().flushAsyncBatchRequests();
    }

    get instance()
    {
        return this._instance;
    }
}

export { Communicator };
