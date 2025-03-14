// Copyright (c) ZeroC, Inc.

#include "EndpointInfo.h"
#include "Ice/Ice.h"
#include "Util.h"

using namespace std;
using namespace IcePy;

namespace IcePy
{
    struct EndpointInfoObject
    {
        PyObject_HEAD Ice::EndpointInfoPtr* endpointInfo;
    };
}

extern "C" EndpointInfoObject*
endpointInfoNew(PyTypeObject* /*type*/, PyObject* /*args*/, PyObject* /*kwds*/)
{
    PyErr_Format(PyExc_RuntimeError, "An endpoint info cannot be created directly");
    return nullptr;
}

extern "C" void
endpointInfoDealloc(EndpointInfoObject* self)
{
    delete self->endpointInfo;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

//
// Ice::EndpointInfo::type
//
extern "C" PyObject*
endpointInfoType(EndpointInfoObject* self, PyObject* /*args*/)
{
    assert(self->endpointInfo);
    return PyLong_FromLong((*self->endpointInfo)->type());
}

//
// Ice::EndpointInfo::datagram
//
extern "C" PyObject*
endpointInfoDatagram(EndpointInfoObject* self, PyObject* /*args*/)
{
    assert(self->endpointInfo);
    return (*self->endpointInfo)->datagram() ? Py_True : Py_False;
}

//
// Ice::EndpointInfo::secure
//
extern "C" PyObject*
endpointInfoSecure(EndpointInfoObject* self, PyObject* /*args*/)
{
    assert(self->endpointInfo);
    return (*self->endpointInfo)->secure() ? Py_True : Py_False;
}

extern "C" PyObject*
endpointInfoGetUnderlying(EndpointInfoObject* self, PyObject* /*args*/)
{
    return createEndpointInfo((*self->endpointInfo)->underlying);
}

extern "C" PyObject*
endpointInfoGetTimeout(EndpointInfoObject* self, PyObject* /*args*/)
{
    return PyLong_FromLong((*self->endpointInfo)->timeout);
}

extern "C" PyObject*
endpointInfoGetCompress(EndpointInfoObject* self, PyObject* /*args*/)
{
    return (*self->endpointInfo)->compress ? Py_True : Py_False;
}

extern "C" PyObject*
ipEndpointInfoGetHost(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::IPEndpointInfo>(*self->endpointInfo);
    assert(info);
    return createString(info->host);
}

extern "C" PyObject*
ipEndpointInfoGetSourceAddress(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::IPEndpointInfo>(*self->endpointInfo);
    assert(info);
    return createString(info->sourceAddress);
}

extern "C" PyObject*
ipEndpointInfoGetPort(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::IPEndpointInfo>(*self->endpointInfo);
    assert(info);
    return PyLong_FromLong(info->port);
}

extern "C" PyObject*
udpEndpointInfoGetMcastInterface(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::UDPEndpointInfo>(*self->endpointInfo);
    assert(info);
    return createString(info->mcastInterface);
}

extern "C" PyObject*
udpEndpointInfoGetMcastTtl(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::UDPEndpointInfo>(*self->endpointInfo);
    assert(info);
    return PyLong_FromLong(info->mcastTtl);
}

extern "C" PyObject*
wsEndpointInfoGetResource(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::WSEndpointInfo>(*self->endpointInfo);
    assert(info);
    return createString(info->resource);
}

extern "C" PyObject*
opaqueEndpointInfoGetRawBytes(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::OpaqueEndpointInfo>(*self->endpointInfo);
    assert(info);
    return PyBytes_FromStringAndSize(
        reinterpret_cast<const char*>(&info->rawBytes[0]),
        static_cast<int>(info->rawBytes.size()));
}

extern "C" PyObject*
opaqueEndpointInfoGetRawEncoding(EndpointInfoObject* self, PyObject* /*args*/)
{
    auto info = dynamic_pointer_cast<Ice::OpaqueEndpointInfo>(*self->endpointInfo);
    assert(info);
    return IcePy::createEncodingVersion(info->rawEncoding);
}

static PyMethodDef EndpointInfoMethods[] = {
    {"type", reinterpret_cast<PyCFunction>(endpointInfoType), METH_NOARGS, PyDoc_STR("type() -> int")},
    {"datagram", reinterpret_cast<PyCFunction>(endpointInfoDatagram), METH_NOARGS, PyDoc_STR("datagram() -> bool")},
    {"secure", reinterpret_cast<PyCFunction>(endpointInfoSecure), METH_NOARGS, PyDoc_STR("secure() -> bool")},
    {0, 0} /* sentinel */
};

static PyGetSetDef EndpointInfoGetters[] = {
    {"underlying",
     reinterpret_cast<getter>(endpointInfoGetUnderlying),
     0,
     PyDoc_STR("underling endpoint information"),
     0},
    {"timeout", reinterpret_cast<getter>(endpointInfoGetTimeout), 0, PyDoc_STR("timeout in milliseconds"), 0},
    {"compress", reinterpret_cast<getter>(endpointInfoGetCompress), 0, PyDoc_STR("compression status"), 0},
    {0, 0} /* sentinel */
};

static PyGetSetDef IPEndpointInfoGetters[] = {
    {"host", reinterpret_cast<getter>(ipEndpointInfoGetHost), 0, PyDoc_STR("host name or IP address"), 0},
    {"port", reinterpret_cast<getter>(ipEndpointInfoGetPort), 0, PyDoc_STR("TCP port number"), 0},
    {"sourceAddress", reinterpret_cast<getter>(ipEndpointInfoGetSourceAddress), 0, PyDoc_STR("source IP address"), 0},
    {0, 0} /* sentinel */
};

static PyGetSetDef UDPEndpointInfoGetters[] = {
    {"mcastInterface",
     reinterpret_cast<getter>(udpEndpointInfoGetMcastInterface),
     0,
     PyDoc_STR("multicast interface"),
     0},
    {"mcastTtl", reinterpret_cast<getter>(udpEndpointInfoGetMcastTtl), 0, PyDoc_STR("multicast time-to-live"), 0},
    {0, 0} /* sentinel */
};

static PyGetSetDef WSEndpointInfoGetters[] = {
    {"resource", reinterpret_cast<getter>(wsEndpointInfoGetResource), 0, PyDoc_STR("resource"), 0},
    {0, 0} /* sentinel */
};

static PyGetSetDef OpaqueEndpointInfoGetters[] = {
    {"rawBytes", reinterpret_cast<getter>(opaqueEndpointInfoGetRawBytes), 0, PyDoc_STR("raw encoding"), 0},
    {"rawEncoding",
     reinterpret_cast<getter>(opaqueEndpointInfoGetRawEncoding),
     0,
     PyDoc_STR("raw encoding version"),
     0},
    {0, 0} /* sentinel */
};

namespace IcePy
{
    PyTypeObject EndpointInfoType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
        .tp_name = "IcePy.EndpointInfo",
        .tp_basicsize = sizeof(EndpointInfoObject),
        .tp_dealloc = reinterpret_cast<destructor>(endpointInfoDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_methods = EndpointInfoMethods,
        .tp_getset = EndpointInfoGetters,
        .tp_new = reinterpret_cast<newfunc>(endpointInfoNew),
    };

    PyTypeObject IPEndpointInfoType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
        .tp_name = "IcePy.IPEndpointInfo",
        .tp_basicsize = sizeof(EndpointInfoObject),
        .tp_dealloc = reinterpret_cast<destructor>(endpointInfoDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_getset = IPEndpointInfoGetters,
        .tp_new = reinterpret_cast<newfunc>(endpointInfoNew),
    };

    PyTypeObject TCPEndpointInfoType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
        .tp_name = "IcePy.TCPEndpointInfo",
        .tp_basicsize = sizeof(EndpointInfoObject),
        .tp_dealloc = reinterpret_cast<destructor>(endpointInfoDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_new = reinterpret_cast<newfunc>(endpointInfoNew),
    };

    PyTypeObject UDPEndpointInfoType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
        .tp_name = "IcePy.UDPEndpointInfo",
        .tp_basicsize = sizeof(EndpointInfoObject),
        .tp_dealloc = reinterpret_cast<destructor>(endpointInfoDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_getset = UDPEndpointInfoGetters,
        .tp_new = reinterpret_cast<newfunc>(endpointInfoNew),
    };

    PyTypeObject WSEndpointInfoType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
        .tp_name = "IcePy.WSEndpointInfo",
        .tp_basicsize = sizeof(EndpointInfoObject),
        .tp_dealloc = reinterpret_cast<destructor>(endpointInfoDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_getset = WSEndpointInfoGetters,
        .tp_new = reinterpret_cast<newfunc>(endpointInfoNew),
    };

    PyTypeObject SSLEndpointInfoType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
        .tp_name = "IcePy.SSLEndpointInfo",
        .tp_basicsize = sizeof(EndpointInfoObject),
        .tp_dealloc = reinterpret_cast<destructor>(endpointInfoDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_new = reinterpret_cast<newfunc>(endpointInfoNew),
    };

    PyTypeObject OpaqueEndpointInfoType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
        .tp_name = "IcePy.OpaqueEndpointInfo",
        .tp_basicsize = sizeof(EndpointInfoObject),
        .tp_dealloc = reinterpret_cast<destructor>(endpointInfoDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .tp_getset = OpaqueEndpointInfoGetters,
        .tp_new = reinterpret_cast<newfunc>(endpointInfoNew),
    };
}

bool
IcePy::initEndpointInfo(PyObject* module)
{
    if (PyType_Ready(&EndpointInfoType) < 0)
    {
        return false;
    }
    PyTypeObject* type = &EndpointInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "EndpointInfo", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    IPEndpointInfoType.tp_base = &EndpointInfoType; // Force inheritance from EndpointInfoType.
    if (PyType_Ready(&IPEndpointInfoType) < 0)
    {
        return false;
    }
    type = &IPEndpointInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "IPEndpointInfo", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    TCPEndpointInfoType.tp_base = &IPEndpointInfoType; // Force inheritance from IPEndpointInfoType.
    if (PyType_Ready(&TCPEndpointInfoType) < 0)
    {
        return false;
    }
    type = &TCPEndpointInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "TCPEndpointInfo", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    UDPEndpointInfoType.tp_base = &IPEndpointInfoType; // Force inheritance from IPEndpointType.
    if (PyType_Ready(&UDPEndpointInfoType) < 0)
    {
        return false;
    }
    type = &UDPEndpointInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "UDPEndpointInfo", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    WSEndpointInfoType.tp_base = &EndpointInfoType; // Force inheritance from IPEndpointType.
    if (PyType_Ready(&WSEndpointInfoType) < 0)
    {
        return false;
    }
    type = &WSEndpointInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "WSEndpointInfo", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    SSLEndpointInfoType.tp_base = &EndpointInfoType; // Force inheritance from IPEndpointInfoType.
    if (PyType_Ready(&SSLEndpointInfoType) < 0)
    {
        return false;
    }
    type = &SSLEndpointInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "SSLEndpointInfo", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    OpaqueEndpointInfoType.tp_base = &EndpointInfoType; // Force inheritance from EndpointType.
    if (PyType_Ready(&OpaqueEndpointInfoType) < 0)
    {
        return false;
    }
    type = &OpaqueEndpointInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "OpaqueEndpointInfo", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    return true;
}

Ice::EndpointInfoPtr
IcePy::getEndpointInfo(PyObject* obj)
{
    assert(PyObject_IsInstance(obj, reinterpret_cast<PyObject*>(&EndpointInfoType)));
    EndpointInfoObject* eobj = reinterpret_cast<EndpointInfoObject*>(obj);
    return *eobj->endpointInfo;
}

PyObject*
IcePy::createEndpointInfo(const Ice::EndpointInfoPtr& endpointInfo)
{
    if (!endpointInfo)
    {
        return Py_None;
    }

    PyTypeObject* type;
    if (dynamic_pointer_cast<Ice::WSEndpointInfo>(endpointInfo))
    {
        type = &WSEndpointInfoType;
    }
    else if (dynamic_pointer_cast<Ice::TCPEndpointInfo>(endpointInfo))
    {
        type = &TCPEndpointInfoType;
    }
    else if (dynamic_pointer_cast<Ice::UDPEndpointInfo>(endpointInfo))
    {
        type = &UDPEndpointInfoType;
    }
    else if (dynamic_pointer_cast<Ice::SSL::EndpointInfo>(endpointInfo))
    {
        type = &SSLEndpointInfoType;
    }
    else if (dynamic_pointer_cast<Ice::OpaqueEndpointInfo>(endpointInfo))
    {
        type = &OpaqueEndpointInfoType;
    }
    else if (dynamic_pointer_cast<Ice::IPEndpointInfo>(endpointInfo))
    {
        type = &IPEndpointInfoType;
    }
    else
    {
        type = &EndpointInfoType;
    }

    EndpointInfoObject* obj = reinterpret_cast<EndpointInfoObject*>(type->tp_alloc(type, 0));
    if (!obj)
    {
        return nullptr;
    }
    obj->endpointInfo = new Ice::EndpointInfoPtr(endpointInfo);

    return (PyObject*)obj;
}
