// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.SeqMapping
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            await using Communicator? communicator = Initialize(ref args);
            IMyClassPrx myClass = AllTests.Run(this, false);
            Console.Out.Write("shutting down server... ");
            Console.Out.Flush();
            await myClass.ShutdownAsync();
            Console.Out.WriteLine("ok");
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
