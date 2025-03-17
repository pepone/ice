// Copyright (c) ZeroC, Inc.

#include "PropertiesAdmin.h"
#include "Ice/DisableWarnings.h"
#include "Thread.h"
#include "Types.h"
#include "Util.h"

using namespace std;
using namespace IcePy;

namespace IcePy
{
    struct NativePropertiesAdminObject
    {
        PyObject_HEAD Ice::NativePropertiesAdminPtr* admin;
        vector<pair<PyObject*, function<void()>>>* callbacks;
    };
}

extern "C" NativePropertiesAdminObject*
nativePropertiesAdminNew(PyTypeObject* /*type*/, PyObject* /*args*/, PyObject* /*kwds*/)
{
    PyErr_Format(PyExc_RuntimeError, "This object cannot be created directly");
    return nullptr;
}

extern "C" void
nativePropertiesAdminDealloc(NativePropertiesAdminObject* self)
{
    delete self->admin;
    delete self->callbacks;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

extern "C" PyObject*
nativePropertiesAdminAddUpdateCB(NativePropertiesAdminObject* self, PyObject* args)
{
    PyObject* callbackType{lookupType("Ice.PropertiesAdminUpdateCallback")};
    PyObject* callback{nullptr};
    if (!PyArg_ParseTuple(args, "O!", callbackType, &callback))
    {
        return nullptr;
    }

    std::function<void()> remover =
        (*self->admin)
            ->addUpdateCallback(
                [callback](const Ice::PropertyDict& dict)
                {
                    AdoptThread adoptThread; // Ensure the current thread is able to call into Python.

                    PyObjectHandle result{PyDict_New()};
                    if (result.get())
                    {
                        for (auto p = dict.begin(); p != dict.end(); ++p)
                        {
                            PyObjectHandle key{createString(p->first)};
                            PyObjectHandle val{createString(p->second)};
                            if (!val.get() || PyDict_SetItem(result.get(), key.get(), val.get()) < 0)
                            {
                                return;
                            }
                        }
                    }

                    PyObjectHandle obj{PyObject_CallMethod(callback, "updated", "O", result.get())};
                    if (!obj.get())
                    {
                        assert(PyErr_Occurred());
                        throw AbortMarshaling();
                    }
                });

    (*self->callbacks).emplace_back(callback, remover);
    Py_INCREF(callback);

    return Py_None;
}

extern "C" PyObject*
nativePropertiesAdminRemoveUpdateCB(NativePropertiesAdminObject* self, PyObject* args)
{
    PyObject* callbackType{lookupType("Ice.PropertiesAdminUpdateCallback")};
    PyObject* callback{nullptr};
    if (!PyArg_ParseTuple(args, "O!", callbackType, &callback))
    {
        return nullptr;
    }

    auto& callbacks = *self->callbacks;

    auto p =
        std::find_if(callbacks.begin(), callbacks.end(), [callback](const auto& q) { return q.first == callback; });
    if (p != callbacks.end())
    {
        p->second();
        Py_DECREF(callback);
        callbacks.erase(p);
    }

    return Py_None;
}

static PyMethodDef NativePropertiesAdminMethods[] = {
    {"addUpdateCallback",
     reinterpret_cast<PyCFunction>(nativePropertiesAdminAddUpdateCB),
     METH_VARARGS,
     PyDoc_STR("addUpdateCallback(callback) -> None")},
    {"removeUpdateCallback",
     reinterpret_cast<PyCFunction>(nativePropertiesAdminRemoveUpdateCB),
     METH_VARARGS,
     PyDoc_STR("removeUpdateCallback(callback) -> None")},
    {nullptr, nullptr} /* sentinel */
};

namespace IcePy
{
    PyTypeObject NativePropertiesAdminType = {
        PyVarObject_HEAD_INIT(nullptr, 0) /* object header */
            .tp_name = "IcePy.NativePropertiesAdmin",
        .tp_basicsize = sizeof(NativePropertiesAdminObject),
        .tp_dealloc = reinterpret_cast<destructor>(nativePropertiesAdminDealloc),
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_methods = NativePropertiesAdminMethods,
        .tp_new = reinterpret_cast<newfunc>(nativePropertiesAdminNew),
    };
}

bool
IcePy::initPropertiesAdmin(PyObject* module)
{
    if (PyType_Ready(&NativePropertiesAdminType) < 0)
    {
        return false;
    }

    if (PyModule_AddObject(module, "NativePropertiesAdmin", reinterpret_cast<PyObject*>(&NativePropertiesAdminType)) <
        0)
    {
        return false;
    }
    return true;
}

PyObject*
IcePy::createNativePropertiesAdmin(const Ice::NativePropertiesAdminPtr& admin)
{
    PyTypeObject* type = &NativePropertiesAdminType;

    auto* p = reinterpret_cast<NativePropertiesAdminObject*>(type->tp_alloc(type, 0));
    if (!p)
    {
        return nullptr;
    }

    p->admin = new Ice::NativePropertiesAdminPtr(admin);
    p->callbacks = new vector<pair<PyObject*, function<void()>>>();
    return (PyObject*)p;
}
