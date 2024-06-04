//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { ObjectAdapterI } from "./ObjectAdapter";
import { Promise } from "./Promise";
import { ObjectAdapterDeactivatedException, AlreadyRegisteredException } from "./LocalException";
import { generateUUID } from "./UUID";

//
// Only for use by Instance.
//
export class ObjectAdapterFactory
{
    constructor(instance, communicator)
    {
        this._instance = instance;
        this._communicator = communicator;
        this._adapters = [];
        this._adapterNamesInUse = [];
        this._shutdownPromise = new Promise();
    }

    shutdown()
    {
        //
        // Ignore shutdown requests if the object adapter factory has
        // already been shut down.
        //
        if(this._instance === null)
        {
            return this._shutdownPromise;
        }

        this._instance = null;
        this._communicator = null;
        Promise.all(this._adapters.map(adapter => adapter.deactivate())).then(() => this._shutdownPromise.resolve());
        return this._shutdownPromise;
    }

    waitForShutdown()
    {
        return this._shutdownPromise.then(() => Promise.all(this._adapters.map(adapter => adapter.waitForDeactivate())));
    }

    isShutdown()
    {
        return this._instance === null;
    }

    destroy()
    {
        return this.waitForShutdown().then(() => Promise.all(this._adapters.map(adapter => adapter.destroy())));
    }

    createObjectAdapter(name, router, promise)
    {
        if(this._instance === null)
        {
            throw new ObjectAdapterDeactivatedException();
        }

        let adapter = null;
        try
        {
            if(name.length === 0)
            {
                adapter = new ObjectAdapterI(this._instance, this._communicator, this, generateUUID(), null, true,
                                             promise);
            }
            else
            {
                if(this._adapterNamesInUse.indexOf(name) !== -1)
                {
                    throw new AlreadyRegisteredException("object adapter", name);
                }
                adapter = new ObjectAdapterI(this._instance, this._communicator, this, name, router, false, promise);
                this._adapterNamesInUse.push(name);
            }
            this._adapters.push(adapter);
        }
        catch(ex)
        {
            promise.reject(ex);
        }
    }

    removeObjectAdapter(adapter)
    {
        if(this._instance === null)
        {
            return;
        }

        let n = this._adapters.indexOf(adapter);
        if(n !== -1)
        {
            this._adapters.splice(n, 1);
        }

        n = this._adapterNamesInUse.indexOf(adapter.getName());
        if(n !== -1)
        {
            this._adapterNamesInUse.splice(n, 1);
        }
    }
}
