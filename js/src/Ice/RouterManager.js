//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { HashMap } from './HashMap.js';
import { Ice as Ice_Router } from './Router.js';
const { RouterPrx } = Ice_Router;
import { RouterInfo } from './RouterInfo.js';

export class RouterManager
{
    constructor()
    {
        this._table = new HashMap(HashMap.compareEquals); // Map<Ice.RouterPrx, RouterInfo>
    }

    destroy()
    {
        for(const router of this._table.values())
        {
            router.destroy();
        }
        this._table.clear();
    }

    //
    // Returns router info for a given router. Automatically creates
    // the router info if it doesn't exist yet.
    //
    find(rtr)
    {
        if(rtr === null)
        {
            return null;
        }

        //
        // The router cannot be routed.
        //
        const router = RouterPrx.uncheckedCast(rtr.ice_router(null));

        let info = this._table.get(router);
        if(info === undefined)
        {
            info = new RouterInfo(router);
            this._table.set(router, info);
        }

        return info;
    }

    erase(rtr)
    {
        let info = null;
        if(rtr !== null)
        {
            // The router cannot be routed.
            const router = RouterPrx.uncheckedCast(rtr.ice_router(null));

            info = this._table.get(router);
            this._table.delete(router);
        }
        return info;
    }
}
