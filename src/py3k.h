#include <Python.h>

#if PY_MAJOR_VERSION >= 3

#define PyString_Check PyUnicode_Check
#define PyString_AsString(val) PyUnicode_AsUTF8(val)
#define PyString_FromString(val) PyUnicode_FromString(val)
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_FromFormat(format, ...) PyUnicode_FromFormat(format, __VA_ARGS__)
#define PyString_Size(val) PyUnicode_GET_SIZE(val)
#define PyInt_Type PyLong_Type
#define PyInt_AS_LONG(val) PyLong_AsLong(val)
#define PyInt_Check(val) PyLong_Check(val)
#define PyInt_FromLong(val) PyLong_FromLong(val)
#define PyInt_AsLong(val) PyLong_AsLong(val)
#define PyInt_FromSize_t(val) PyLong_FromSize_t(val)

#endif

#ifndef PyVarObject_HEAD_INIT
  #define PyVarObject_HEAD_INIT(type, size) \
          PyObject_HEAD_INIT(type) size,
#endif
