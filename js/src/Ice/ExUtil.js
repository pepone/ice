//
// Copyright (c) ZeroC, Inc. All rights reserved.
//


import { UnexpectedObjectException, MemoryLimitException } from "./LocalException";

//
// Exception utilities
//
const ExUtil =
{
    throwUOE: function(expectedType, v)
    {
        const type = v.ice_id();
        throw new UnexpectedObjectException("expected element of type `" + expectedType + "' but received `" +
                                            type + "'", type, expectedType);
    },
    throwMemoryLimitException: function(requested, maximum)
    {
        throw new MemoryLimitException("requested " + requested + " bytes, maximum allowed is " + maximum +
                                       " bytes (see Ice.MessageSizeMax)");
    }
};

export { ExUtil };
