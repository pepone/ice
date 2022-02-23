//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Exception } from "./Exception";

let Debug = {}

if (typeof process !== 'undefined')
{
    /* eslint no-sync: "off" */
    /* eslint no-process-exit: "off" */
    const fs = require("fs");
    Debug = class
    {
        static assert(b, msg)
        {
            if(!b)
            {
                fs.writeSync(process.stderr.fd, msg === undefined ? "assertion failed" : msg);
                fs.writeSync(process.stderr.fd, new Error().stack);
                process.exit(1);
            }
        }
    }
    /* eslint no-sync: "on" */
    /* eslint no-process-exit: "on" */
}
else
{
    class AssertionFailedException extends Error
    {
        constructor(message)
        {
            super();
            Exception.captureStackTrace(this);
            this.message = message;
        }
    }

    Debug = class
    {
        static assert(b, msg)
        {
            if(!b)
            {
                console.log(msg === undefined ? "assertion failed" : msg);
                console.log(Error().stack);
                throw new AssertionFailedException(msg === undefined ? "assertion failed" : msg);
            }
        }
    }
}

export { Debug };