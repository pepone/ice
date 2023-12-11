//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Reflection;
using System.Linq;
using System.Threading.Tasks;

[assembly: CLSCompliant(true)]

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Client : Test.TestHelper
{
    public override void run(string[] args)
    {
        using(var communicator = initialize(ref args))
        {
            if(args.Any(v => v.Equals("--with-deploy")))
            {
                AllTests.allTestsWithDeploy(this);
            }
            else
            {
                AllTests.allTests(this);
            }
        }
    }

    public static Task<int> Main(string[] args) =>
        Test.TestDriver.runTestAsync<Client>(args);
}
