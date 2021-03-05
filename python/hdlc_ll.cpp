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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"
#include "proto/hdlc/low_level/hdlc.h"
#include "hdlc_ll.h"

typedef struct
{
    PyObject_HEAD // no semicolon
    hdlc_ll_handle_t handle;
    Py_buffer user_buffer;
    hdlc_crc_t crc_type;
    int mtu;
    void *buffer;
    PyObject *on_frame_sent;
    PyObject *on_frame_read;
} Hdlc;

static PyMemberDef Hdlc_members[] =
{
    {"mtu", T_INT, offsetof( Hdlc, mtu ), 0, "Maximum size of payload"},
    {NULL} /* Sentinel */
};

/////////////////////////////// ALLOC/DEALLOC

static void Hdlc_dealloc(Hdlc *self)
{
    if ( self->user_buffer.buf )
    {
         PyBuffer_Release( &self->user_buffer );
    }
    if ( self->on_frame_read != NULL )
    {
         Py_DECREF( self->on_frame_read );
         self->on_frame_read = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int Hdlc_init(Hdlc *self, PyObject *args, PyObject *kwds)
{
    self->handle = NULL;
    self->buffer = NULL;
    self->crc_type = HDLC_CRC_16;
    self->user_buffer.buf = NULL;
    self->on_frame_sent = NULL;
    self->on_frame_read = NULL;
    self->mtu = 1500;
    return 0;
}

static PyObject *Hdlc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Hdlc *self;

    self = (Hdlc *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        Hdlc_init( self, args, kwds );
    }
    return (PyObject *)self;
}

////////////////////////////// Internal callbacks

static int on_frame_read( void *user_data, void *data, int len )
{
    int result = 0;
    Hdlc *self = (Hdlc *)user_data;
    if ( self->on_frame_read )
    {
        PyObject *arg = PyByteArray_FromStringAndSize( (const char *)data, (Py_ssize_t)len );
        PyObject *temp = PyObject_CallFunctionObjArgs( self->on_frame_read, arg, NULL );
        if ( temp && PyLong_Check( temp ) )
        {
            result = PyLong_AsLong( temp );
        }
        Py_XDECREF( temp ); // Dereference result
        Py_DECREF( arg );  // We do not need ByteArray anymore
    }
    return result;
}

static int on_frame_sent( void *user_data, const void *data, int len )
{
    int result = 0;
    Hdlc *self = (Hdlc *)user_data;
    if ( self->on_frame_sent )
    {
        PyObject *arg = PyByteArray_FromStringAndSize( (const char *)data, (Py_ssize_t)len );
        // We can free Py_buffer user_buffer only after we constructed new PyObject with the data
        if ( self->user_buffer.buf )
        {
            PyBuffer_Release( &self->user_buffer );
            self->user_buffer.buf = NULL;
        }
        PyObject *temp = PyObject_CallFunctionObjArgs( self->on_frame_sent, arg, NULL );
        if ( temp && PyLong_Check( temp ) )
        {
            result = PyLong_AsLong( temp );
        }
        Py_XDECREF( temp ); // Dereference result
        Py_DECREF( arg );  // We do not need ByteArray anymore
    }
    else
    {
        if ( self->user_buffer.buf )
        {
            PyBuffer_Release( &self->user_buffer );
            self->user_buffer.buf = NULL;
        }
    }
    return result;
}

////////////////////////////// METHODS

static PyObject *Hdlc_begin(Hdlc *self)
{
    hdlc_ll_init_t init{};
    init.user_data = self;
    init.crc_type = self->crc_type;
    init.on_frame_read = on_frame_read;
    init.on_frame_sent = on_frame_sent;
    init.buf_size = hdlc_ll_get_buf_size( self->mtu );
    init.buf = malloc( init.buf_size );
    int result = hdlc_ll_init( &self->handle, &init );
    return PyLong_FromLong((long)result);
}

static PyObject *Hdlc_end(Hdlc *self)
{
    hdlc_ll_close( self->handle );
    self->handle = NULL;
    Py_RETURN_NONE;
}

static PyObject *Hdlc_put(Hdlc *self, PyObject *args)
{
    Py_buffer      buffer{};
    if (!PyArg_ParseTuple(args, "s*", &buffer))
    {
        return NULL;
    }
    int result = hdlc_ll_put( self->handle, buffer.buf, buffer.len );
    if ( result == TINY_ERR_BUSY || result == TINY_ERR_INVALID_DATA )
    {
        PyBuffer_Release( &buffer );
    }
    else
    {
        if ( self->user_buffer.buf )
        {
            PyBuffer_Release( &self->user_buffer );
            self->user_buffer.buf = NULL;
        }
        // Save user data until send operation is complete
        self->user_buffer = buffer;
    }
    return PyLong_FromLong((long)result);
}

static PyObject *Hdlc_rx(Hdlc *self, PyObject *args)
{
    Py_buffer      buffer{};
    PyObject     * error = NULL;
    if (!PyArg_ParseTuple(args, "s*|O", &buffer, &error))
    {
        return NULL;
    }
    int err_code;
    int result = hdlc_ll_run_rx( self->handle, buffer.buf, buffer.len, &err_code );
    PyBuffer_Release( &buffer );
    return PyLong_FromLong((long)result);
}

static PyObject *Hdlc_tx(Hdlc *self, PyObject *args)
{
    int result;
    Py_buffer      buffer{};
    if (!PyArg_ParseTuple(args, "|s*", &buffer))
    {
        return NULL;
    }
    if ( buffer.buf == NULL )
    {
        void * data = PyObject_Malloc( self->mtu );
        result = hdlc_ll_run_tx( self->handle, data, self->mtu );
        PyObject     * to_send = PyByteArray_FromStringAndSize( (const char *)data, result );
        PyObject_Free( data );
        return to_send;
    }
    else
    {
        result = hdlc_ll_run_tx( self->handle, buffer.buf, buffer.len );
        PyBuffer_Release( &buffer );
        return PyLong_FromLong((long)result);
    }
}

///////////////////////////////// GETTERS SETTERS

static PyObject * Hdlc_get_on_read(Hdlc *self, void *closure)
{
    Py_INCREF(self->on_frame_read);
    return self->on_frame_read;
}

static int Hdlc_set_on_read(Hdlc *self, PyObject *value, void *closure)
{
    PyObject * tmp = self->on_frame_read;
    Py_INCREF( value );
    self->on_frame_read = value;
    Py_XDECREF( tmp );
    return 0;
}

static PyObject * Hdlc_get_on_send(Hdlc *self, void *closure)
{
    Py_INCREF(self->on_frame_sent);
    return self->on_frame_sent;
}

static int Hdlc_set_on_send(Hdlc *self, PyObject *value, void *closure)
{
    PyObject * tmp = self->on_frame_sent;
    Py_INCREF( value );
    self->on_frame_sent = value;
    Py_XDECREF( tmp );
    return 0;
}

static PyGetSetDef Hdlc_getsetters[] = {
    {"on_read", (getter) Hdlc_get_on_read, (setter) Hdlc_set_on_read, "Callback for incoming messages", NULL},
    {"on_send", (getter) Hdlc_get_on_send, (setter) Hdlc_set_on_send, "Callback for successfully sent messages", NULL},
    {NULL}  /* Sentinel */
};

///////////////////////////////// BINDINGS

static PyMethodDef Hdlc_methods[] =
{
    {"begin", (PyCFunction)Hdlc_begin, METH_NOARGS, "Initializes Hdlc protocol"},
    {"end", (PyCFunction)Hdlc_end, METH_NOARGS, "Stops Hdlc protocol"},
    {"put", (PyCFunction)Hdlc_put, METH_VARARGS, "Puts new message for sending"},
    {"rx", (PyCFunction)Hdlc_rx, METH_VARARGS, "Passes rx data"},
    {"tx", (PyCFunction)Hdlc_tx, METH_VARARGS, "Fills specified buffer with tx data"},
    {NULL} /* Sentinel */
};

PyTypeObject HdlcType = {
    PyVarObject_HEAD_INIT(NULL, 0) "tinyproto.Hdlc",  /* tp_name */
    sizeof(Hdlc),                             /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Hdlc_dealloc,                 /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash  */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Hdlc object",                            /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Hdlc_methods,                             /* tp_methods */
    Hdlc_members,                             /* tp_members */
    Hdlc_getsetters,                          /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)Hdlc_init,                      /* tp_init */
    0,                                        /* tp_alloc */
    Hdlc_new,                                 /* tp_new */
};


