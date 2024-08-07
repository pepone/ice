// Copyright (c) ZeroC, Inc.

import Ice
import TestCommon

public class Client: TestHelperI {
    override public func run(args: [String]) async throws {
        let properties = try createTestProperties(args)
        var initData = Ice.InitializationData()
        initData.properties = properties
        initData.classResolverPrefix = ["IceInvoke"]

        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }
        let cl = try await allTests(self)
        try await cl.shutdown()
    }
}
