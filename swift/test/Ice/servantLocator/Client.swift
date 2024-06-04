//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Ice
import TestCommon

public class Client: TestHelperI {
    override public func run(args: [String]) throws {
        var initData = Ice.InitializationData()
        initData.properties = try createTestProperties(args)
        initData.classResolverPrefix = ["IceServantLocator"]
        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }
        let obj = try allTests(self)
        try obj.shutdown()
    }
}
