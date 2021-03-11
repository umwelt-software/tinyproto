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
    PyObject *read_func;
    PyObject *write_func;
    int error_flag;
} Fd;

static PyMemberDef Fd_members[] =
{
    {"mtu", T_INT, offsetof( Fd, mtu ), 0, "Maximum size of payload"},
    {NULL} /* Sentinel */
};

/////////////////////////////// ALLOC/DEALLOC

static void Fd_dealloc(Fd *self)
{
    Py_XDECREF( self->on_frame_read );
    Py_XDECREF( self->on_frame_sent );
    Py_XDECREF( self->read_func );
    Py_XDECREF( self->write_func );
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int Fd_init(Fd *self, PyObject *args, PyObject *kwds)
{
    self->handle = NULL;
    self->buffer = NULL;
    self->crc_type = HDLC_CRC_16;
    self->on_frame_sent = NULL;
    self->on_frame_read = NULL;
    self->read_func = NULL;
    self->write_func = NULL;
    self->mtu = 1500;
    self->window_size = 7;
    self->error_flag = 0;
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
    init.buffer_size = tiny_fd_buffer_size_by_mtu_ex( self->mtu, self->window_size, init.crc_type );
    self->buffer = PyObject_Malloc( init.buffer_size );
    init.buffer = self->buffer;
    init.send_timeout = 1000;
    init.retry_timeout = 200;
    init.retries = 2;
    init.window_frames = self->window_size;
    init.mtu = self->mtu;
    int result = tiny_fd_init( &self->handle, &init );
    return PyLong_FromLong((long)result);
}

static PyObject *Fd_end(Fd *self)
{
    tiny_fd_close( self->handle );
    self->handle = NULL;
    PyObject_Free( self->buffer );
    self->buffer = NULL;
    Py_RETURN_NONE;
}

static PyObject *Fd_send(Fd *self, PyObject *args)
{
    Py_buffer      buffer{};
    if (!PyArg_ParseTuple(args, "s*", &buffer))
    {
        return NULL;
    }
    int result = tiny_fd_send_packet( self->handle, buffer.buf, buffer.len );
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

int write_func(void *user_data, const void *buffer, int size)
{
    int result = 0;
    Fd *self = (Fd *)user_data;
    if ( self->write_func )
    {
        PyObject *arg = PyByteArray_FromStringAndSize( (const char *)buffer, (Py_ssize_t)size );
        PyObject *temp = PyObject_CallFunctionObjArgs( self->write_func, arg, NULL );
        if ( !temp || !PyLong_Check( temp ) )
        {
            Py_XDECREF( temp ); // Dereference result
            Py_DECREF( arg );  // We do not need ByteArray anymore
            self->error_flag = 1;
            return size;
        }
        result = PyLong_AsLong( temp );
        Py_XDECREF( temp ); // Dereference result
        Py_DECREF( arg );  // We do not need ByteArray anymore
    }
    return result;
}

static PyObject *Fd_run_tx(Fd *self, PyObject *args)
{
    int result = 0;
    PyObject * cb = NULL;
    if (!PyArg_ParseTuple(args, "O", &cb))
    {
        return NULL;
    }
    if ( !PyFunction_Check( cb ) )
    {
        return NULL;
    }
    Py_INCREF( cb );
    self->write_func = cb;
    result = tiny_fd_run_tx( self->handle, write_func );
    Py_DECREF( cb );
    self->write_func = NULL;
    if ( self->error_flag )
    {
        return PyErr_Format( PyExc_RuntimeError, "Write function must return integer number of bytes written" );
    }
    return PyLong_FromLong((long)result);
}

int read_func(void *user_data, void *buffer, int size)
{
    int result = 0;
    Fd *self = (Fd *)user_data;
    if ( self->read_func )
    {
        PyObject *arg = PyLong_FromLong((long)size);
        PyObject *temp = PyObject_CallFunctionObjArgs( self->read_func, arg, NULL );
        if ( !temp || !PyObject_CheckBuffer( temp ) )
        {
            Py_XDECREF( temp ); // Dereference result
            Py_DECREF( arg );  // We do not need ByteArray anymore
            self->error_flag = 1;
            return 0;
        }
        Py_buffer view;
        if ( PyObject_GetBuffer( temp, &view, 0 ) >= 0 )
        {
            memcpy( buffer, view.buf, view.len );
            result = view.len;
            PyBuffer_Release( &view );
        }
        Py_XDECREF( temp ); // Dereference result
        Py_DECREF( arg );
    }
    return result;
}

static PyObject *Fd_run_rx(Fd *self, PyObject *args)
{
    int result = 0;
    PyObject * cb = NULL;
    if (!PyArg_ParseTuple(args, "O", &cb))
    {
        return NULL;
    }
    if ( !PyFunction_Check( cb ) )
    {
        return NULL;
    }
    Py_INCREF( cb );
    self->read_func = cb;
    result = tiny_fd_run_rx( self->handle, read_func );
    Py_DECREF( cb );
    self->read_func = NULL;
    if ( self->error_flag )
    {
        return PyErr_Format( PyExc_RuntimeError, "Read function must return bytearray with the bytes" );
    }
    return PyLong_FromLong((long)result);
}


/*
void tiny_fd_set_ka_timeout 	( 	tiny_fd_handle_t  	handle,
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
    {"send", (PyCFunction)Fd_send, METH_VARARGS, "Sends new message to remote side"},
    {"rx", (PyCFunction)Fd_rx, METH_VARARGS, "Passes rx data"},
    {"tx", (PyCFunction)Fd_tx, METH_VARARGS, "Fills specified buffer with tx data"},
    {"run_rx", (PyCFunction)Fd_run_rx, METH_VARARGS, "Reads data from user callback and parses them"},
    {"run_tx", (PyCFunction)Fd_run_tx, METH_VARARGS, "Writes data to user callback"},
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


