//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using Test;

namespace ZeroC.Ice.Test.Info
{
    public class TestIntf : ITestIntf
    {
        public void shutdown(Current current) => current.Adapter.Communicator.Shutdown();

        public IReadOnlyDictionary<string, string> getEndpointInfoAsContext(Current current)
        {
            TestHelper.Assert(current.Connection != null);
            var ctx = new Dictionary<string, string>();
            Endpoint endpoint = current.Connection.Endpoint;
            ctx["timeout"] = endpoint.Timeout.ToString();
            ctx["compress"] = endpoint.HasCompressionFlag ? "true" : "false";
            ctx["datagram"] = endpoint.IsDatagram ? "true" : "false";
            ctx["secure"] = endpoint.IsDatagram ? "true" : "false";
            ctx["type"] = endpoint.Type.ToString();

            IPEndpoint? ipEndpoint = endpoint as IPEndpoint;
            TestHelper.Assert(ipEndpoint != null);
            ctx["host"] = ipEndpoint.Host;
            ctx["port"] = ipEndpoint.Port.ToString();

            if (ipEndpoint is UdpEndpoint udpEndpoint)
            {
                ctx["mcastInterface"] = udpEndpoint.McastInterface;
                ctx["mcastTtl"] = udpEndpoint.McastTtl.ToString();
            }

            return ctx;
        }

        public IReadOnlyDictionary<string, string> getConnectionInfoAsContext(Current current)
        {
            TestHelper.Assert(current.Connection != null);
            var ctx = new Dictionary<string, string>();
            ConnectionInfo info = current.Connection.GetConnectionInfo();
            System.Console.WriteLine($"Adapter Name: {info.AdapterName!}");
            ctx["adapterName"] = info.AdapterName!;
            ctx["incoming"] = info.Incoming ? "true" : "false";

            var ipinfo = info as IpConnectionInfo;
            TestHelper.Assert(ipinfo != null);
            ctx["localAddress"] = ipinfo.LocalAddress;
            ctx["localPort"] = ipinfo.LocalPort.ToString();
            ctx["remoteAddress"] = ipinfo.RemoteAddress;
            ctx["remotePort"] = ipinfo.RemotePort.ToString();

            if (info is WsConnectionInfo || info is WssConnectionInfo)
            {
                var headers = info is WsConnectionInfo wsInfo ? wsInfo.Headers! : ((WssConnectionInfo)info).Headers!;
                foreach (KeyValuePair<string, string> e in headers)
                {
                    ctx["ws." + e.Key] = e.Value;
                }
            }

            return ctx;
        }
    }
}
