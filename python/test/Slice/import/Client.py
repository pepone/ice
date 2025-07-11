#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

import sys

import Test
from TestHelper import TestHelper


def test(b):
    if not b:
        raise RuntimeError("test assertion failed")


class Client(TestHelper):
    def run(self, args):
        sys.stdout.write("testing imports... ")
        sys.stdout.flush()

        test(Test.SubA.SubSubA1.Value1 == 10)
        test(Test.SubA.SubSubA1.Value2 == 11)
        test(Test.SubA.SubSubA2.Value1 == 30)
        test(Test.SubB.SubSubB1.Value1 == 20)
        test(Test.SubB.SubSubB1.Value2 == 21)

        print("ok")
