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

static void Object_objectAddKey(void *prv, JSOBJ obj, JSOBJ name, JSOBJ value)
{
  PyDict_SetItem (obj, name, value);
  Py_DECREF( (PyObject *) name);
  Py_DECREF( (PyObject *) value);
  return;
}

static void Object_arrayAddItem(void *prv, JSOBJ obj, JSOBJ value)
{
  PyList_Append(obj, value);
  Py_DECREF( (PyObject *) value);
  return;
}

static JSOBJ Object_newString(void *prv, wchar_t *start, wchar_t *end)
{
  return PyUnicode_FromWideChar (start, (end - start));
}

static JSOBJ Object_newTrue(void *prv)
{
  Py_RETURN_TRUE;
}

static JSOBJ Object_newFalse(void *prv)
{
  Py_RETURN_FALSE;
}

static JSOBJ Object_newNull(void *prv)
{
  Py_RETURN_NONE;
}

static JSOBJ Object_newObject(void *prv)
{
  return PyDict_New();
}

static JSOBJ Object_newArray(void *prv)
{
  return PyList_New(0);
}

static JSOBJ Object_newInteger(void *prv, JSINT32 value)
{
  return PyLong_FromLong( (long) value);
}

static JSOBJ Object_newLong(void *prv, JSINT64 value)
{
  return PyLong_FromLongLong (value);
}

static JSOBJ Object_newUnsignedLong(void *prv, JSUINT64 value)
{
  return PyLong_FromUnsignedLongLong (value);
}

static JSOBJ Object_newDouble(void *prv, double value)
{
  return PyFloat_FromDouble(value);
}

static void Object_releaseObject(void *prv, JSOBJ obj)
{
  Py_DECREF( ((PyObject *)obj));
}

//static char *g_kwlist[] = {"obj", NULL};

HPy_DEF_METH_O(JSONToObj)
static HPy
JSONToObj_impl(HPyContext ctx, HPy self, HPy arg)
{
  PyObject *ret;
  HPy sarg;
  HPy h_ret;
  JSONObjectDecoder decoder =
  {
    Object_newString,
    Object_objectAddKey,
    Object_arrayAddItem,
    Object_newTrue,
    Object_newFalse,
    Object_newNull,
    Object_newObject,
    Object_newArray,
    Object_newInteger,
    Object_newLong,
    Object_newUnsignedLong,
    Object_newDouble,
    Object_releaseObject,
    PyObject_Malloc,
    PyObject_Free,
    PyObject_Realloc
  };

  decoder.prv = NULL;

  if (HPyBytes_Check(ctx, arg))
  {
    sarg = HPy_Dup(ctx, arg);
  }
  else
  if (HPyUnicode_Check(ctx, arg))
  {
    sarg = HPyUnicode_AsUTF8String(ctx, arg);
    if (HPy_IsNull(sarg))
    {
      //Exception raised above us by codec according to docs
      return HPy_NULL;
    }
  }
  else
  {
    HPyErr_SetString(ctx, ctx->h_TypeError, "Expected String or Unicode");
    return HPy_NULL;
  }

  decoder.errorStr = NULL;
  decoder.errorOffset = NULL;

  dconv_s2d_init(DCONV_S2D_ALLOW_TRAILING_JUNK, 0.0, 0.0, "Infinity", "NaN");

  ret = JSON_DecodeObject(&decoder,
                          HPyBytes_AS_STRING(ctx, sarg),
                          HPyBytes_GET_SIZE(ctx, sarg));


  dconv_s2d_free();

  HPy_Close(ctx, sarg);

  if (decoder.errorStr)
  {
    /*
    FIXME: It's possible to give a much nicer error message here with actual failing element in input etc*/

    // XXX hpy does not support HPyErr_Format yet!
    //PyErr_Format (PyExc_ValueError, "%s", decoder.errorStr);
    HPyErr_SetString(ctx, ctx->h_ValueError, "generic error while decoding");

    if (ret)
    {
        Py_DECREF( (PyObject *) ret);
    }

    return HPy_NULL;
  }

  h_ret = HPy_FromPyObject(ctx, ret);
  Py_DECREF(ret);
  return h_ret;
}

HPy_DEF_METH_O(JSONFileToObj)
static HPy
JSONFileToObj_impl(HPyContext ctx, HPy self, HPy h_arg)
{
  PyObject *file = HPy_AsPyObject(ctx, h_arg);
  PyObject *read;
  PyObject *string;
  HPy h_result;

  if (!PyObject_HasAttrString (file, "read"))
  {
    HPyErr_SetString(ctx, ctx->h_TypeError, "expected file");
    return HPy_NULL;
  }

  read = PyObject_GetAttrString (file, "read");

  if (!PyCallable_Check (read)) {
    Py_XDECREF(read);
    HPyErr_SetString(ctx, ctx->h_TypeError, "expected file");
    return HPy_NULL;
  }

  string = PyObject_CallObject (read, NULL);
  Py_XDECREF(read);

  if (string == NULL)
  {
    return HPy_NULL;
  }

  HPy h_string = HPy_FromPyObject(ctx, string);
  h_result = JSONToObj_impl(ctx, self, h_string);
  Py_XDECREF(string);
  HPy_Close(ctx, h_string);

  if (HPy_IsNull(h_result)) {
    return HPy_NULL;
  }
  return h_result;
}
