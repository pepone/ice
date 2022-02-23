
//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Logger } from "./LoggerI";

let processLogger = null;

function getProcessLogger()
{
    if(processLogger === null)
    {
        //
        // TODO: Would be nice to be able to use process name as prefix by default.
        //
        processLogger = new Logger("", "");
    }

    return processLogger;
};

function setProcessLogger(logger)
{
    processLogger = logger;
};

export { getProcessLogger, setProcessLogger };
