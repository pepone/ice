#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

import TestI
from TestHelper import TestHelper

import AllTests
import Ice


class Collocated(TestHelper):
    def run(self, args):
        with self.initialize(args=args) as communicator:
            communicator.getProperties().setProperty("TestAdapter.Endpoints", self.getTestEndpoint())
            adapter = communicator.createObjectAdapter("TestAdapter")
            adapter.add(TestI.DI(), Ice.stringToIdentity("d"))
            adapter.addFacet(TestI.DI(), Ice.stringToIdentity("d"), "facetABCD")
            adapter.addFacet(TestI.FI(), Ice.stringToIdentity("d"), "facetEF")
            adapter.addFacet(TestI.HI(communicator), Ice.stringToIdentity("d"), "facetGH")

            # adapter.activate() // Don't activate OA to ensure collocation is used.

            AllTests.allTests(self, communicator)
