#include <Python.h>

#if PY_MAJOR_VERSION >= 3

#define PyInt_FromLong(v) PyLong_FromLong(v)
#define PyInt_AS_LONG(v) PyLong_AsLong(v)
#define PyInt_AsLong(v) PyLong_AsLong(v)
#define PyInt_Type PyLong_Type
#define PyInt_Check(o) PyLong_Check(o)
#define PyInt_FromSize_t(v) PyLong_FromSize_t(v)

#define PyString_FromString(v) PyUnicode_FromString(v)
#define PyString_Size(o) PyUnicode_GET_SIZE(o)
#define PyString_FromFormat(fmt, ...) PyUnicode_FromFormat(fmt, __VA_ARGS__)
#define PyString_Check(o) PyUnicode_Check(o)

#if PY_MINOR_VERSION >= 3
#define PyString_AsString PyUnicode_AsUTF8
#else
const char* PyUnicode_AsUTF8_PriorToPy33(PyObject* unicode);
#define PyString_AsString(o) PyUnicode_AsUTF8_PriorToPy33(o)
#endif

#endif

#ifndef PyVarObject_HEAD_INIT

#define PyVarObject_HEAD_INIT(type, size) \
PyObject_HEAD_INIT(type) size,

#endif

#ifndef Py_TYPE

#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)

#endif