/*
Developed by ESN, an Electronic Arts Inc. studio. 
Copyright (c) 2014, Electronic Arts Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of ESN, Electronic Arts Inc. nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS INC. BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Portions of code from MODP_ASCII - Ascii transformations (upper/lower, etc)
http://code.google.com/p/stringencoders/
Copyright (c) 2007  Nick Galbreath -- nickg [at] modp [dot] com. All rights reserved.

Numeric decoder derived from from TCL library
http://www.opensource.apple.com/source/tcl/tcl-14/tcl/license.terms
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
*/

#include "py_defines.h"
#include <ultrajson.h>


//#define PRINTMARK() fprintf(stderr, "%s: MARK(%d)\n", __FILE__, __LINE__)
#define PRINTMARK()

JSOBJ ujdecode_newString(void *prv, wchar_t *start, wchar_t *end)
{
  return PyUnicode_FromWideChar (start, (end - start));
}

void ujdecode_objectAddKey(void *prv, JSOBJ obj, JSOBJ name, JSOBJ value)
{
  PyDict_SetItem (obj, name, value);
  Py_DECREF( (PyObject *) name);
  Py_DECREF( (PyObject *) value);
  return;
}

void ujdecode_arrayAddItem(void *prv, JSOBJ obj, JSOBJ value)
{
  PyList_Append(obj, value);
  Py_DECREF( (PyObject *) value);
  return;
}

JSOBJ ujdecode_newTrue(void *prv)
{
  Py_RETURN_TRUE;
}

JSOBJ ujdecode_newFalse(void *prv)
{
  Py_RETURN_FALSE;
}

JSOBJ ujdecode_newNull(void *prv)
{
  Py_RETURN_NONE;
}

JSOBJ ujdecode_newObject(void *prv)
{
  return PyDict_New();
}

JSOBJ ujdecode_newArray(void *prv)
{
  return PyList_New(0);
}

JSOBJ ujdecode_newInt(void *prv, JSINT32 value)
{
  return PyInt_FromLong( (long) value);
}

JSOBJ ujdecode_newLong(void *prv, JSINT64 value)
{
  return PyLong_FromLongLong (value);
}

JSOBJ ujdecode_newUnsignedLong(void *prv, JSUINT64 value)
{
  return PyLong_FromUnsignedLongLong (value);
}

JSOBJ ujdecode_newDouble(void *prv, double value)
{
  return PyFloat_FromDouble(value);
}

void ujdecode_releaseObject(void *prv, JSOBJ obj)
{
  Py_DECREF( ((PyObject *)obj));
}

void *ujdecode_malloc(size_t size)
{
  return PyObject_Malloc(size);
}

void ujdecode_free(void *pptr)
{
  PyObject_Free(pptr);
}

void *ujdecode_realloc(void *base, size_t size)
{
  return PyObject_Realloc(base, size);
}

static char *g_kwlist[] = {"obj", "precise_float", NULL};

PyObject* JSONToObj(PyObject* self, PyObject *args, PyObject *kwargs)
{
  PyObject *ret;
  PyObject *sarg;
  PyObject *arg;
  PyObject *opreciseFloat = NULL;

  JSONObjectDecoder decoder;
  
  decoder.preciseFloat = 0;
  decoder.prv = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", g_kwlist, &arg, &opreciseFloat))
  {
      return NULL;
  }

  if (opreciseFloat && PyObject_IsTrue(opreciseFloat))
  {
      decoder.preciseFloat = 1;
  }

  if (PyString_Check(arg))
  {
      sarg = arg;
  }
  else
  if (PyUnicode_Check(arg))
  {
    sarg = PyUnicode_AsUTF8String(arg);
    if (sarg == NULL)
    {
      //Exception raised above us by codec according to docs
      return NULL;
    }
  }
  else
  {
    PyErr_Format(PyExc_TypeError, "Expected String or Unicode");
    return NULL;
  }

  decoder.errorStr = NULL;
  decoder.errorOffset = NULL;

  ret = JSON_DecodeObject(&decoder, PyString_AS_STRING(sarg), PyString_GET_SIZE(sarg));

  if (sarg != arg)
  {
    Py_DECREF(sarg);
  }

  if (decoder.errorStr)
  {
    /*
    FIXME: It's possible to give a much nicer error message here with actual failing element in input etc*/

    PyErr_Format (PyExc_ValueError, "%s", decoder.errorStr);

    if (ret)
    {
        Py_DECREF( (PyObject *) ret);
    }

    return NULL;
  }

  return ret;
}

PyObject* JSONFileToObj(PyObject* self, PyObject *args, PyObject *kwargs)
{
  PyObject *read;
  PyObject *string;
  PyObject *result;
  PyObject *file = NULL;
  PyObject *argtuple;

  if (!PyArg_ParseTuple (args, "O", &file))
  {
    return NULL;
  }

  if (!PyObject_HasAttrString (file, "read"))
  {
    PyErr_Format (PyExc_TypeError, "expected file");
    return NULL;
  }

  read = PyObject_GetAttrString (file, "read");

  if (!PyCallable_Check (read)) {
    Py_XDECREF(read);
    PyErr_Format (PyExc_TypeError, "expected file");
    return NULL;
  }

  string = PyObject_CallObject (read, NULL);
  Py_XDECREF(read);

  if (string == NULL)
  {
    return NULL;
  }

  argtuple = PyTuple_Pack(1, string);

  result = JSONToObj (self, argtuple, kwargs);

  Py_XDECREF(argtuple);
  Py_XDECREF(string);

  if (result == NULL) {
    return NULL;
  }

  return result;
}
