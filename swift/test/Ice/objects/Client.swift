// Copyright (c) ZeroC, Inc.

import Ice
import PromiseKit
import TestCommon

public class Client: TestHelperI {
    override public func run(args: [String]) throws {
        let properties = try createTestProperties(args)
        properties.setProperty(key: "Ice.AcceptClassCycles", value: "1")
        var initData = InitializationData()
        initData.properties = properties
        initData.classResolverPrefix = ["IceObjects"]
        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }
        try communicator.getValueFactoryManager().add(factory: { _ in BI() }, id: "::Test::B")
        try communicator.getValueFactoryManager().add(factory: { _ in CI() }, id: "::Test::C")
        try communicator.getValueFactoryManager().add(factory: { _ in DI() }, id: "::Test::D")
        try communicator.getValueFactoryManager().add(factory: { _ in EI() }, id: "::Test::E")
        try communicator.getValueFactoryManager().add(factory: { _ in FI() }, id: "::Test::F")
        try communicator.getValueFactoryManager().add(factory: { _ in II() }, id: "::Test::I")
        try communicator.getValueFactoryManager().add(factory: { _ in JI() }, id: "::Test::J")

        let initial = try allTests(self)
        try initial.shutdown()
    }
}
