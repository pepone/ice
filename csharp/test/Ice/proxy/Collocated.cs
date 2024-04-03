//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Threading.Tasks;
using Test;

namespace Ice
{
    namespace proxy
    {
        public class Collocated : TestHelper
        {
            public override async Task runAsync(string[] args)
            {
                var properties = createTestProperties(ref args);
                properties.setProperty("Ice.ThreadPool.Client.Size", "2"); // For nested AMI.
                properties.setProperty("Ice.ThreadPool.Client.SizeWarn", "0");
                properties.setProperty("Ice.Warn.Dispatch", "0");

                using (var communicator = initialize(properties))
                {
                    communicator.getProperties().setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                    var adapter = communicator.createObjectAdapter("TestAdapter");
                    adapter.add(new MyDerivedClassI(), Ice.Util.stringToIdentity("test"));
                    //adapter.activate(); // Don't activate OA to ensure collocation is used.
                    await AllTests.allTests(this);
                }
            }

            public static Task<int> Main(String[] args) =>
                TestDriver.runTestAsync<Collocated>(args);
        }
    }
}
