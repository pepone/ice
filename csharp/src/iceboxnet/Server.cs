// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;

namespace IceBox
{
    public static class Server
    {
        public static int Main(string[] args)
        {
            try
            {
                using var communicator = new Ice.Communicator(ref args, new Dictionary<string, string>()
                                                              {
                                                                  { "Ice.Admin.DelayCreation", "1" }
                                                              });

                Console.CancelKeyPress += (sender, eventArgs) =>
                {
                    eventArgs.Cancel = true;
                    communicator.Shutdown();
                };

                return Run(communicator, args);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
                return 1;
            }
        }

        private static void Usage()
        {
            Console.Error.WriteLine("Usage: iceboxnet [options] --Ice.Config=<file>\n");
            Console.Error.WriteLine("Options:");
            Console.Error.WriteLine("-h, --help           Show this message.");
            Console.Error.WriteLine("-v, --version        Display the Ice version.");
        }

        private static int Run(Ice.Communicator communicator, string[] args)
        {
            const string prefix = "IceBox.Service.";
            Dictionary<string, string> services = communicator.GetProperties(forPrefix: prefix);

            var argSeq = new List<string>(args);
            foreach (KeyValuePair<string, string> pair in services)
            {
                string name = pair.Key.Substring(prefix.Length);
                argSeq.RemoveAll(v => v.StartsWith("--" + name));
            }

            foreach (string arg in args)
            {
                if (arg.Equals("-h") || arg.Equals("--help"))
                {
                    Usage();
                    return 0;
                }
                else if (arg.Equals("-v") || arg.Equals("--version"))
                {
                    Console.Out.WriteLine(Ice.Util.StringVersion());
                    return 0;
                }
                else
                {
                    Console.Error.WriteLine("IceBox.Server: unknown option `" + arg + "'");
                    Usage();
                    return 1;
                }
            }

            var serviceManagerImpl = new ServiceManager(communicator, args);
            return serviceManagerImpl.Run();
        }
    }
}
