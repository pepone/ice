# Copyright (c) ZeroC, Inc. All rights reserved.

# ruff: noqa: F401, F821, E402, F403

"""
Ice module
"""

import IcePy
from .ModuleUtil import *
from .EnumBase import EnumBase

#
# Add some symbols to the Ice module.
#
ObjectPrx = IcePy.ObjectPrx
stringVersion = IcePy.stringVersion
intVersion = IcePy.intVersion
currentProtocol = IcePy.currentProtocol
currentProtocolEncoding = IcePy.currentProtocolEncoding
currentEncoding = IcePy.currentEncoding
stringToProtocolVersion = IcePy.stringToProtocolVersion
protocolVersionToString = IcePy.protocolVersionToString
stringToEncodingVersion = IcePy.stringToEncodingVersion
encodingVersionToString = IcePy.encodingVersionToString
generateUUID = IcePy.generateUUID
loadSlice = IcePy.loadSlice
AsyncInvocationContext = IcePy.AsyncInvocationContext
Unset = IcePy.Unset

#
# Forward declarations.
#
IcePy._t_Object = IcePy.declareClass("::Ice::Object")
IcePy._t_Value = IcePy.declareValue("::Ice::Object")
IcePy._t_ObjectPrx = IcePy.declareProxy("::Ice::Object")

#
# Import local definitions that are part of the Ice module public API.
#
from .Future import *
from .InvocationFuture import *
from .Value import *
from .Object import *
from .Blobject import *
from .BlobjectAsync import *
from .FormatType import *
from .PropertiesAdminUpdateCallback import *
from .Util import *
from .UnknownSlicedValue import *
from .ToStringMode import *
from .Exception import *
from .LocalException import *
from .UserException import *
from .Communicator import *
from .ImplicitContext import *
from .EndpointSelectionType import *
from .ObjectAdapter import *
from .ValueFactory import *
from .ConnectionClose import *
from .CompressBatch import *
from .ServantLocator import *
from .InitializationData import *
from .Properties import *
from .Logger import *
from .BatchRequestInterceptor import *
from .LocalExceptions import *
from .Proxy import *

#
# Import the generated code for the Ice module.
#
import Ice.BuiltinSequences_ice
import Ice.OperationMode_ice
import Ice.EndpointTypes_ice
import Ice.Identity_ice
import Ice.Locator_ice
import Ice.Process_ice
import Ice.PropertiesAdmin_ice
import Ice.RemoteLogger_ice
import Ice.Router_ice
import Ice.Version_ice
import Ice.Metrics_ice

#
# Add EndpointInfo alias in Ice module.
#
EndpointInfo = IcePy.EndpointInfo
IPEndpointInfo = IcePy.IPEndpointInfo
TCPEndpointInfo = IcePy.TCPEndpointInfo
UDPEndpointInfo = IcePy.UDPEndpointInfo
WSEndpointInfo = IcePy.WSEndpointInfo
OpaqueEndpointInfo = IcePy.OpaqueEndpointInfo
SSLEndpointInfo = IcePy.SSLEndpointInfo

#
# Add ConnectionInfo alias in Ice module.
#
ConnectionInfo = IcePy.ConnectionInfo
IPConnectionInfo = IcePy.IPConnectionInfo
TCPConnectionInfo = IcePy.TCPConnectionInfo
UDPConnectionInfo = IcePy.UDPConnectionInfo
WSConnectionInfo = IcePy.WSConnectionInfo
SSLConnectionInfo = IcePy.SSLConnectionInfo

#
# Protocol and Encoding constants
#
Protocol_1_0 = Ice.ProtocolVersion(1, 0)
Encoding_1_0 = Ice.EncodingVersion(1, 0)
Encoding_1_1 = Ice.EncodingVersion(1, 1)

#
# Native PropertiesAdmin admin facet.
#
NativePropertiesAdmin = IcePy.NativePropertiesAdmin

#
# This value is used as the default value for struct types in the constructors
# of user-defined types. It allows us to determine whether the application has
# supplied a value. (See bug 3676)
#
# TODO: can we use None instead?
_struct_marker = object()
