# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

from Util import ClientTestCase, SimpleClient, TestSuite


TestSuite(__name__, [ClientTestCase(client=SimpleClient(args=["{testdir}"]))])
