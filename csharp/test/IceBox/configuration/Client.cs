//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using ZeroC.Ice;
using Test;
using System.Threading.Tasks;

namespace ZeroC.IceBox.Test.Configuration
{
    public class Client : TestHelper
    {
        public override Task Run(string[] args)
        {
            var properties = CreateTestProperties(ref args);
            properties["Ice.Default.Host"] = "127.0.0.1";
            using var communicator = Initialize(properties);
            AllTests.allTests(this);
            // Shutdown the IceBox server.
            IProcessPrx.Parse("DemoIceBox/admin -f Process:default -p 9996", communicator).Shutdown();
            return Task.CompletedTask;
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
