#include <Python.h>
#include "2to3.h"

#if PY_MAJOR_VERSION >= 3
#if PY_MINOR_VERSION < 3
const char* PyUnicode_AsUTF8_PriorToPy33(PyObject* value)
{
    if (PyUnicode_Check(value)) {
        value = PyUnicode_AsUTF8String(value);
        if (value == NULL) {
            return NULL;
        }
    }
    return PyBytes_AsString(value);
}
#endif
#endif
