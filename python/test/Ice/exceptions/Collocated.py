#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

import Ice
from TestHelper import TestHelper
import TestI
import AllTests


class Collocated(TestHelper):
    def run(self, args):
        properties = self.createTestProperties(args)
        properties.setProperty("Ice.MessageSizeMax", "10")

        with self.initialize(properties=properties) as communicator:
            communicator.getProperties().setProperty("Ice.Warn.Dispatch", "0")
            communicator.getProperties().setProperty(
                "TestAdapter.Endpoints", self.getTestEndpoint()
            )
            adapter = communicator.createObjectAdapter("TestAdapter")
            adapter.add(TestI.ThrowerI(), Ice.stringToIdentity("thrower"))
            # adapter.activate() // Don't activate OA to ensure collocation is used.
            AllTests.allTests(self, communicator)
