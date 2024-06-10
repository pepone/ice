// Copyright (c) ZeroC, Inc.

import Dispatch
import Ice
import TestCommon

class ServantLocatorI: Ice.ServantLocator {
    var _blobject: Ice.Dispatcher

    init(_ async: Bool) {
        if async {
            _blobject = Ice.BlobjectAsyncDisp(BlobjectAsyncI())
        } else {
            _blobject = Ice.BlobjectDisp(BlobjectI())
        }
    }

    func locate(_: Ice.Current) -> (returnValue: Ice.Dispatcher?, cookie: AnyObject?) {
        return (_blobject, nil)
    }

    func finished(curr _: Ice.Current, servant _: Ice.Dispatcher, cookie _: AnyObject?) {}

    func deactivate(_: String) {}
}

class Server: TestHelperI {
    override public func run(args: [String]) throws {
        let async = args.contains("--async")

        let properties = try createTestProperties(args)
        var initData = Ice.InitializationData()
        initData.properties = properties
        initData.classResolverPrefix = ["IceInvoke"]
        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }

        communicator.getProperties().setProperty(
            key: "TestAdapter.Endpoints", value: "\(getTestEndpoint(num: 0))")
        let adapter = try communicator.createObjectAdapter("TestAdapter")

        try adapter.addServantLocator(locator: ServantLocatorI(async), category: "")
        try adapter.activate()

        try adapter.activate()
        serverReady()
        communicator.waitForShutdown()
    }
}
