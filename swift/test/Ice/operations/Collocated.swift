//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Ice
import TestCommon

class Collocated: TestHelperI {
    public override func run(args: [String]) throws {
        let properties = try createTestProperties(args)

        properties.setProperty(key: "Ice.ThreadPool.Client.Size", value: "2")
        properties.setProperty(key: "Ice.ThreadPool.Client.SizeWarn", value: "0")
        properties.setProperty(key: "Ice.BatchAutoFlushSize", value: "100")

        //
        // Its possible to have batch oneway requests dispatched
        // after the adapter is deactivated due to thread
        // scheduling so we supress this warning.
        //
        properties.setProperty(key: "Ice.Warn.Dispatch", value: "0")
        //
        // We don't want connection warnings because of the timeout test.
        //
        properties.setProperty(key: "Ice.Warn.Connections", value: "0")

        var initData = Ice.InitializationData()
        initData.properties = properties
        initData.classResolverPrefix = ["IceOperations"]
        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }

        communicator.getProperties().setProperty(key: "TestAdapter.Endpoints", value: getTestEndpoint(num: 0))
        let adapter = try communicator.createObjectAdapter("TestAdapter")
        try adapter.add(servant: MBDisp(BI()), id: Ice.stringToIdentity("b"))
        try adapter.add(servant: MyDerivedClassDisp(MyDerivedClassI(self)),
                        id: Ice.stringToIdentity("test"))
        // try adapter.activate() // Don't activate OA to ensure collocation is used.
        _ = try allTests(helper: self)
    }
}
