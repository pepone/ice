// Copyright (c) ZeroC, Inc.

import Foundation
import Ice
import TestCommon

class Server: TestHelperI {
    override public func run(args: [String]) throws {
        let communicator = try initialize(args)
        defer {
            communicator.destroy()
        }

        var path = Bundle.main.bundlePath
        #if os(iOS) || os(watchOS) || os(tvOS)
            path += "/Frameworks/IceSSLConfiguration.bundle/certs"
        #else
            path += "/Contents/Frameworks/IceSSLConfiguration.bundle/Contents/Resources/certs"
        #endif
        communicator.getProperties().setProperty(
            key: "TestAdapter.Endpoints",
            value: getTestEndpoint(num: 0, prot: "tcp"))
        let adapter = try communicator.createObjectAdapter("TestAdapter")
        try adapter.add(
            servant: SSLServerFactoryDisp(ServerFactoryI(defaultDir: path, helper: self)),
            id: Ice.stringToIdentity("factory"))
        try adapter.activate()
        serverReady()
        communicator.waitForShutdown()
    }
}
