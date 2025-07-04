#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

from TestHelper import TestHelper
import AllTests


class Client(TestHelper):
    def run(self, args):
        properties = self.createTestProperties(args)
        properties.setProperty("Ice.MessageSizeMax", "10")
        properties.setProperty("Ice.Warn.Connections", "0")
        with self.initialize(properties=properties) as communicator:
            thrower = AllTests.allTests(self, communicator)
            thrower.shutdown()
