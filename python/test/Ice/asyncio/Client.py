#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

import asyncio
from TestHelper import TestHelper
TestHelper.loadSlice("Test.ice")
import AllTests


class Client(TestHelper):

    def run(self, args):

        async def runAsync():
            properties = self.createTestProperties(args)
            loop = asyncio.get_running_loop()
            with self.initialize(properties=properties, eventLoop=loop) as communicator:
                await AllTests.allTestsAsync(self, communicator)

        asyncio.run(runAsync(), debug=True)
