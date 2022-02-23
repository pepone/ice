//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { StringUtil } from "./StringUtil";

class HashUtil
{
    static addBoolean(h, b)
    {
        return ((h << 5) + h) ^ (b ? 0 : 1);
    }

    static addString(h, str)
    {
        if(str !== undefined && str !== null)
        {
            h = ((h << 5) + h) ^ StringUtil.hashCode(str);
        }
        return h;
    }

    static addNumber(h, num)
    {
        return ((h << 5) + h) ^ num;
    }

    static addHashable(h, obj)
    {
        if(obj !== undefined && obj !== null)
        {
            h = ((h << 5) + h) ^ obj.hashCode();
        }
        return h;
    }

    static addArray(h, arr, hashCode)
    {
        if(arr !== undefined && arr !== null)
        {
            for(let i = 0; i < arr.length; ++i)
            {
                h = hashCode(h, arr[i]);
            }
        }
        return h;
    }
}

export { HashUtil };
