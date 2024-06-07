//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Ice } from "ice";

class ControllerHelper
{
    write(msg)
    {
        process.stdout.write(msg);
    }

    writeLine(msg)
    {
        process.stdout.write(msg + "\n");
    }

    serverReady()
    {
        console.log("server ready");
    }
}

(async function()
 {
     try
     {
         const name = process.argv[2];
         const module = await import(name);
         const cls = module[name];
         const test = new cls();
         test.setControllerHelper(new ControllerHelper());
         await test.run(process.argv);
     }
     catch(ex)
     {
         console.log(ex);
         /* eslint-disable no-process-exit */
         process.exit(1);
         /* eslint-enable no-process-exit */
     }
 }());
