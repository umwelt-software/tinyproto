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
//#include "proto/hdlc/low_level/hdlc.h"
#include "proto/fd/tiny_fd.h"
#include "fd.h"

typedef struct
{
    PyObject_HEAD // no semicolon
    tiny_fd_handle_t handle;
    hdlc_crc_t crc_type;
    int mtu;
    int window_size;
    void *buffer;
    PyObject *on_frame_sent;
    PyObject *on_frame_read;
} Fd;

static PyMemberDef Fd_members[] =
{
    {"mtu", T_INT, offsetof( Fd, mtu ), 0, "Maximum size of payload"},
    {NULL} /* Sentinel */
};

/////////////////////////////// ALLOC/DEALLOC

static void Fd_dealloc(Fd *self)
{
    if ( self->on_frame_read != NULL )
    {
         Py_DECREF( self->on_frame_read );
         self->on_frame_read = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int Fd_init(Fd *self, PyObject *args, PyObject *kwds)
{
    self->handle = NULL;
    self->buffer = NULL;
    self->crc_type = HDLC_CRC_16;
    self->on_frame_sent = NULL;
    self->on_frame_read = NULL;
    self->mtu = 1500;
    self->window_size = 7;
    return 0;
}

static PyObject *Fd_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Fd *self;

    self = (Fd *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        Fd_init( self, args, kwds );
    }
    return (PyObject *)self;
}

////////////////////////////// Internal callbacks

static void on_frame_read( void *user_data, uint8_t *data, int len )
{
    Fd *self = (Fd *)user_data;
    if ( self->on_frame_read )
    {
        PyObject *arg = PyByteArray_FromStringAndSize( (const char *)data, (Py_ssize_t)len );
        PyObject *temp = PyObject_CallFunctionObjArgs( self->on_frame_read, arg, NULL );
        Py_XDECREF( temp ); // Dereference result
        Py_DECREF( arg );  // We do not need ByteArray anymore
    }
}

static void on_frame_sent( void *user_data, uint8_t *data, int len )
{
    Fd *self = (Fd *)user_data;
    if ( self->on_frame_sent )
    {
        PyObject *arg = PyByteArray_FromStringAndSize( (const char *)data, (Py_ssize_t)len );
        PyObject *temp = PyObject_CallFunctionObjArgs( self->on_frame_sent, arg, NULL );
        Py_XDECREF( temp ); // Dereference result
        Py_DECREF( arg );  // We do not need ByteArray anymore
    }
}

////////////////////////////// METHODS

static PyObject *Fd_begin(Fd *self)
{
    tiny_fd_init_t init{};
    init.pdata = self;
    init.on_frame_cb = on_frame_read;
    init.on_sent_cb = on_frame_sent;
    init.crc_type = self->crc_type;
    init.buffer_size = tiny_fd_buffer_size_by_mtu( self->mtu, self->window_size );
    // TODO: Don't forget to free the memory
    init.buffer = malloc( init.buffer_size );
    init.send_timeout = 1000;
    init.retry_timeout = 200;
    init.retries = 2;
    init.window_frames = self->window_size;
    int result = tiny_fd_init( &self->handle, &init );
    return PyLong_FromLong((long)result);
}

static PyObject *Fd_end(Fd *self)
{
    tiny_fd_close( self->handle );
    self->handle = NULL;
    Py_RETURN_NONE;
}

static PyObject *Fd_put(Fd *self, PyObject *args)
{
    Py_buffer      buffer{};
    if (!PyArg_ParseTuple(args, "s*", &buffer))
    {
        return NULL;
    }
    int result = tiny_fd_send( self->handle, buffer.buf, buffer.len );
    PyBuffer_Release( &buffer );
    return PyLong_FromLong((long)result);
}

static PyObject *Fd_rx(Fd *self, PyObject *args)
{
    Py_buffer      buffer{};
    if (!PyArg_ParseTuple(args, "s*", &buffer))
    {
        return NULL;
    }
    int result = tiny_fd_on_rx_data( self->handle, buffer.buf, buffer.len );
    PyBuffer_Release( &buffer );
    return PyLong_FromLong((long)result);
}

static PyObject *Fd_tx(Fd *self, PyObject *args)
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
        result = tiny_fd_get_tx_data( self->handle, data, self->mtu );
        PyObject     * to_send = PyByteArray_FromStringAndSize( (const char *)data, result );
        PyObject_Free( data );
        return to_send;
    }
    else
    {
        result = tiny_fd_get_tx_data( self->handle, buffer.buf, buffer.len );
        PyBuffer_Release( &buffer );
        return PyLong_FromLong((long)result);
    }
}

/*
oid tiny_fd_set_ka_timeout 	( 	tiny_fd_handle_t  	handle,
		uint32_t  	keep_alive
	)
*/

///////////////////////////////// GETTERS SETTERS

static PyObject * Fd_get_on_read(Fd *self, void *closure)
{
    Py_INCREF(self->on_frame_read);
    return self->on_frame_read;
}

static int Fd_set_on_read(Fd *self, PyObject *value, void *closure)
{
    PyObject * tmp = self->on_frame_read;
    Py_INCREF( value );
    self->on_frame_read = value;
    Py_XDECREF( tmp );
    return 0;
}

static PyObject * Fd_get_on_send(Fd *self, void *closure)
{
    Py_INCREF(self->on_frame_sent);
    return self->on_frame_sent;
}

static int Fd_set_on_send(Fd *self, PyObject *value, void *closure)
{
    PyObject * tmp = self->on_frame_sent;
    Py_INCREF( value );
    self->on_frame_sent = value;
    Py_XDECREF( tmp );
    return 0;
}

static PyGetSetDef Fd_getsetters[] = {
    {"on_read", (getter) Fd_get_on_read, (setter) Fd_set_on_read, "Callback for incoming messages", NULL},
    {"on_send", (getter) Fd_get_on_send, (setter) Fd_set_on_send, "Callback for successfully sent messages", NULL},
    {NULL}  /* Sentinel */
};

///////////////////////////////// BINDINGS

static PyMethodDef Fd_methods[] =
{
    {"begin", (PyCFunction)Fd_begin, METH_NOARGS, "Initializes Fd protocol"},
    {"end", (PyCFunction)Fd_end, METH_NOARGS, "Stops Fd protocol"},
    {"put", (PyCFunction)Fd_put, METH_VARARGS, "Puts new message for sending"},
    {"rx", (PyCFunction)Fd_rx, METH_VARARGS, "Passes rx data"},
    {"tx", (PyCFunction)Fd_tx, METH_VARARGS, "Fills specified buffer with tx data"},
    {NULL} /* Sentinel */
};

PyTypeObject FdType = {
    PyVarObject_HEAD_INIT(NULL, 0) "tinyproto.Fd",  /* tp_name */
    sizeof(Fd),                               /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Fd_dealloc,                   /* tp_dealloc */
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
    "Fd object",                              /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Fd_methods,                               /* tp_methods */
    Fd_members,                               /* tp_members */
    Fd_getsetters,                            /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)Fd_init,                        /* tp_init */
    0,                                        /* tp_alloc */
    Fd_new,                                   /* tp_new */
};


