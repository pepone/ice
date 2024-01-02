# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
#
# Ice version 3.7.10
#
# <auto-generated>
#
# Generated from file `InstrumentationF.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

from sys import version_info as _version_info_
import Ice, IcePy

# Start of module Ice
_M_Ice = Ice.openModule('Ice')
__name__ = 'Ice'

# Start of module Ice.Instrumentation
_M_Ice.Instrumentation = Ice.openModule('Ice.Instrumentation')
__name__ = 'Ice.Instrumentation'

if 'Observer' not in _M_Ice.Instrumentation.__dict__:
    _M_Ice.Instrumentation._t_Observer = IcePy.declareValue('::Ice::Instrumentation::Observer')

if 'CommunicatorObserver' not in _M_Ice.Instrumentation.__dict__:
    _M_Ice.Instrumentation._t_CommunicatorObserver = IcePy.declareValue('::Ice::Instrumentation::CommunicatorObserver')

# End of module Ice.Instrumentation

__name__ = 'Ice'

# End of module Ice
