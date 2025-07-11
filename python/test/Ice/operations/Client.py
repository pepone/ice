#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

from TestHelper import TestHelper

import AllTests


class Client(TestHelper):
    def run(self, args):
        properties = self.createTestProperties(args)
        properties.setProperty("Ice.ThreadPool.Client.Size", "2")
        properties.setProperty("Ice.ThreadPool.Client.SizeWarn", "0")
        properties.setProperty("Ice.BatchAutoFlushSize", "100")

        with self.initialize(properties=properties) as communicator:
            AllTests.allTests(self, communicator)
            # Test multiple destroy calls
            communicator.destroy()
            communicator.destroy()
