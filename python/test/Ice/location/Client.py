#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

from TestHelper import TestHelper

import AllTests


class Client(TestHelper):
    def run(self, args):
        properties = self.createTestProperties(args)
        properties.setProperty(
            "Ice.Default.Locator",
            "locator:{0}".format(self.getTestEndpoint(properties=properties)),
        )
        with self.initialize(properties=properties) as communicator:
            AllTests.allTests(self, communicator)
