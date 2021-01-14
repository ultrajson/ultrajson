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
  HPyContext ctx = (HPyContext)prv;
  HPy h_obj = HPy_FromVoidP(obj);
  HPy h_name = HPy_FromVoidP(name);
  HPy h_value = HPy_FromVoidP(value);
  HPy_SetItem(ctx, h_obj, h_name, h_value);
  HPy_Close(ctx, h_name);
  HPy_Close(ctx, h_value);
  return;
}

static void Object_arrayAddItem(void *prv, JSOBJ obj, JSOBJ value)
{
  HPyContext ctx = (HPyContext)prv;
  HPy h_obj = HPy_FromVoidP(obj);
  HPy h_value = HPy_FromVoidP(value);
  HPyList_Append(ctx, h_obj, h_value);
  HPy_Close(ctx, h_value);
  return;
}

static JSOBJ Object_newString(void *prv, wchar_t *start, wchar_t *end)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPyUnicode_FromWideChar(ctx, start, (end - start));
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newTrue(void *prv)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPy_Dup(ctx, ctx->h_True);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newFalse(void *prv)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPy_Dup(ctx, ctx->h_False);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newNull(void *prv)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPy_Dup(ctx, ctx->h_None);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newObject(void *prv)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPyDict_New(ctx);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newArray(void *prv)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPyList_New(ctx, 0);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newInteger(void *prv, JSINT32 value)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPyLong_FromLong(ctx, (long) value);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newLong(void *prv, JSINT64 value)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPyLong_FromLongLong(ctx, value);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newUnsignedLong(void *prv, JSUINT64 value)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPyLong_FromUnsignedLongLong(ctx, value);
  return HPy_AsVoidP(res);
}

static JSOBJ Object_newDouble(void *prv, double value)
{
  HPyContext ctx = (HPyContext)prv;
  HPy res = HPyFloat_FromDouble(ctx, value);
  return HPy_AsVoidP(res);
}

static void Object_releaseObject(void *prv, JSOBJ obj)
{
  HPyContext ctx = (HPyContext)prv;
  HPy h_obj = HPy_FromVoidP(obj);
  HPy_Close(ctx, h_obj);
}

//static char *g_kwlist[] = {"obj", NULL};

HPyDef_METH(JSONToObj, "loads", JSONToObj_impl, HPyFunc_O,
            .doc="Converts JSON as string to dict object structure. "
                 "Use precise_float=True to use high precision float decoder.");

// ujson.c does something a bit weird: it defines two Python-level methods
// ("encode" and "loads") for the same C-level function
// ("JSONToObj_impl"). HPyDef_METH does not support this use case, but we can
// define our HPyDef by hand: HPyDef_METH is just a convenience macro and the
// structure and fields of HPyDef is part of the publich API
HPyDef JSONToObj_decode = {
    .kind = HPyDef_Kind_Meth,
    .meth = {
        .name = "decode",
        .impl = JSONToObj_impl,
        .cpy_trampoline = JSONToObj_trampoline,
        .signature = HPyFunc_O,
        .doc = ("Converts JSON as string to dict object structure. "
                "Use precise_float=True to use high precision float decoder."),
    }
};

static HPy
JSONToObj_impl(HPyContext ctx, HPy self, HPy arg)
{
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

  // use decoder.prv to pass around the ctx
  decoder.prv = ctx;

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

  void *ret = JSON_DecodeObject(&decoder,
                                HPyBytes_AS_STRING(ctx, sarg),
                                HPyBytes_GET_SIZE(ctx, sarg));
  h_ret = HPy_FromVoidP(ret);

  dconv_s2d_free();

  HPy_Close(ctx, sarg);

  if (decoder.errorStr)
  {
    /*
    FIXME: It's possible to give a much nicer error message here with actual failing element in input etc*/

    // XXX hpy does not support HPyErr_Format yet!
    //PyErr_Format (PyExc_ValueError, "%s", decoder.errorStr);
    HPyErr_SetString(ctx, ctx->h_ValueError, "generic error while decoding");

    if (!HPy_IsNull(h_ret))
    {
      HPy_Close(ctx, h_ret);
    }

    return HPy_NULL;
  }

  return h_ret;
}

HPyDef_METH(JSONFileToObj, "load", JSONFileToObj_impl, HPyFunc_O,
            .doc="Converts JSON as file to dict object structure. "
                 "Use precise_float=True to use high precision float decoder.")
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
