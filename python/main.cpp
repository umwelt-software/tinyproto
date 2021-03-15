/*
    Copyright 2021 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Python.h>
#include "hdlc_ll.h"
#include "fd.h"

static PyObject *pants(PyObject *self, PyObject *args)
{
    int input;
    if ( !PyArg_ParseTuple(args, "i", &input) )
    {
        return NULL;
    }

    return PyLong_FromLong((long)input * (long)input);
}

static PyMethodDef tinyproto_methods[] = {{"pants", pants, METH_VARARGS, "Returns a square of an integer."},
                                          {NULL, NULL, 0, NULL}};

static struct PyModuleDef tinyproto_definition = {PyModuleDef_HEAD_INIT, "tinyproto", "A Python tiny protocol module",
                                                  -1, tinyproto_methods};

PyMODINIT_FUNC PyInit_tinyproto(void)
{
    Py_Initialize();
    PyObject *m = PyModule_Create(&tinyproto_definition);

    if ( PyType_Ready(&HdlcType) < 0 )
    {
        return NULL;
    }

    if ( PyType_Ready(&FdType) < 0 )
    {
        return NULL;
    }

    Py_INCREF(&HdlcType);
    PyModule_AddObject(m, "Hdlc", (PyObject *)&HdlcType);
    Py_INCREF(&FdType);
    PyModule_AddObject(m, "Fd", (PyObject *)&FdType);

    return m;
}
