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
https://code.google.com/p/stringencoders/
Copyright (c) 2007  Nick Galbreath -- nickg [at] modp [dot] com. All rights reserved.

Numeric decoder derived from from TCL library
https://www.opensource.apple.com/source/tcl/tcl-14/tcl/license.terms
* Copyright (c) 1988-1993 The Regents of the University of California.
* Copyright (c) 1994 Sun Microsystems, Inc.
*/

#include <Python.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

#include "ultrajson.h"

/*
Worst cases being:

Control characters (ASCII < 32)
0x00 (1 byte) input => \u0000 output (6 bytes)
1 * 6 => 6 (6 bytes required)

or UTF-16 surrogate pairs
4 bytes input in UTF-8 => \uXXXX\uYYYY (12 bytes).

4 * 6 => 24 bytes (12 bytes required)

The extra 2 bytes are for the quotes around the string

*/
#define RESERVE_STRING(_len) (2 + ((_len) * 6))

static const char g_hexChars[] = "0123456789abcdef";
static const char g_escapeChars[] = "0123456789\\b\\t\\n\\f\\r\\\"\\\\\\/";

struct __TypeContext;
typedef int (*JSPFN_ITERNEXT)(PyObject *obj, struct __TypeContext *tc);
typedef void (*JSPFN_ITEREND)(PyObject *obj, struct __TypeContext *tc);
typedef PyObject *(*JSPFN_ITERGETVALUE)(PyObject *obj, struct __TypeContext *tc);
typedef char *(*JSPFN_ITERGETNAME)(PyObject *obj, struct __TypeContext *tc, size_t *outLen);
static char *JSON_EncodeObject(PyObject *obj, struct __JSONObjectEncoder *enc, char *buffer, size_t cbBuffer, size_t *outLen);
typedef void *(*PFN_PyTypeToJSON)(PyObject *obj, struct __TypeContext *tc, void *outValue, size_t *_outLen);
bool object_is_decimal_type(PyObject *obj);

typedef struct __TypeContext
{
  int type;
  JSPFN_ITEREND iterEnd;
  JSPFN_ITERNEXT iterNext;
  JSPFN_ITERGETNAME iterGetName;
  JSPFN_ITERGETVALUE iterGetValue;
  PFN_PyTypeToJSON PyTypeToJSON;
  PyObject *newObj;
  PyObject *utf8BytesObj;
  PyObject *dictObj;
  Py_ssize_t index;
  Py_ssize_t size;
  PyObject *itemValue;
  PyObject *itemName;

  union
  {
    PyObject *rawJSONValue;
    int64_t longValue;
    uint64_t unsignedLongValue;
  };
} TypeContext;

// If newObj is set, we should use it rather than the original PyObject
#define GET_OBJ(__pyobj, __ptrtc) ((__ptrtc)->newObj ? (__ptrtc)->newObj : __pyobj)

// Avoid infinite loop caused by the default function
#define DEFAULT_FN_MAX_DEPTH 3

//#define PRINTMARK() fprintf(stderr, "%s: MARK(%d)\n", __FILE__, __LINE__)
#define PRINTMARK()

static void *PyLongToINT64(PyObject *unused, TypeContext *tc, void *outValue, size_t *_outLen)
{
  *((int64_t *) outValue) = tc->longValue;
  return NULL;
}

static void *PyLongToUINT64(PyObject *unused, TypeContext *tc, void *outValue, size_t *_outLen)
{
  *((uint64_t *) outValue) = tc->unsignedLongValue;
  return NULL;
}

static void *PyLongToINTSTR(PyObject *unused, TypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = tc->rawJSONValue;
  *_outLen = PyUnicode_GET_LENGTH(obj);
  return PyUnicode_1BYTE_DATA(obj);
}

static void *PyFloatToDOUBLE(PyObject *obj, TypeContext *tc, void *outValue, size_t *_outLen)
{
  *((double *) outValue) = PyFloat_AsDouble (obj);
  return NULL;
}

static void *PyStringToUTF8(PyObject *obj, TypeContext *tc, void *outValue, size_t *_outLen)
{
  *_outLen = PyBytes_GET_SIZE(obj);
  return PyBytes_AS_STRING(obj);
}

static char *PyUnicodeToUTF8Raw(PyObject *obj, size_t *_outLen, PyObject **pBytesObj)
{
  /*
  Converts the PyUnicode object to char* whose size is stored in _outLen.
  This conversion may require the creation of an intermediate PyBytes object.
  In that case, the returned char* is in fact the internal buffer of that PyBytes object,
  and when the char* buffer is no longer needed, the bytesObj must be DECREF'd.
  */
#ifndef Py_LIMITED_API
  if (PyUnicode_IS_COMPACT_ASCII(obj))
  {
    Py_ssize_t len;
    char *data = (char *) PyUnicode_AsUTF8AndSize(obj, &len);
    *_outLen = len;
    return data;
  }
#endif

  Py_XDECREF(*pBytesObj);
  PyObject *bytesObj = *pBytesObj = PyUnicode_AsEncodedString (obj, NULL, "surrogatepass");
  if (!bytesObj)
  {
    return NULL;  // Out of memory
  }

  *_outLen = PyBytes_GET_SIZE(bytesObj);
  return PyBytes_AS_STRING(bytesObj);
}

static void *PyUnicodeToUTF8(PyObject *_obj, TypeContext *tc, void *outValue, size_t *_outLen)
{
  return PyUnicodeToUTF8Raw(_obj, _outLen, &(tc->utf8BytesObj));
}

static void *PyRawJSONToUTF8(PyObject *unused, TypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = tc->rawJSONValue;
  if (PyUnicode_Check(obj))
  {
    return PyUnicodeToUTF8(obj, tc, outValue, _outLen);
  }
  else
  {
    return PyStringToUTF8(obj, tc, outValue, _outLen);
  }
}

static int Tuple_iterNext(PyObject *obj, TypeContext *tc)
{
  if (tc->index >= tc->size)
  {
    return 0;
  }

  tc->itemValue = PyTuple_GET_ITEM(obj, tc->index);
  tc->index ++;
  return 1;
}

static void Tuple_iterEnd(PyObject *obj, TypeContext *tc)
{
}

static PyObject *Tuple_iterGetValue(PyObject *obj, TypeContext *tc)
{
  return tc->itemValue;
}

static int List_iterNext(PyObject *obj, TypeContext *tc)
{
  if (tc->index >= tc->size)
  {
    PRINTMARK();
    return 0;
  }

  tc->itemValue = PyList_GET_ITEM(obj, tc->index);
  tc->index ++;
  return 1;
}

static void List_iterEnd(PyObject *obj, TypeContext *tc)
{
}

static PyObject *List_iterGetValue(PyObject *obj, TypeContext *tc)
{
  return tc->itemValue;
}

//=============================================================================
// Dict iteration functions
// itemName might converted to string (PyObject_Str). Do refCounting
// itemValue is borrowed from object (which is dict). No refCounting
//=============================================================================

static PyObject* Dict_convertKey(PyObject* key)
{
  if (PyUnicode_Check(key))
  {
    return PyUnicode_AsEncodedString(key, NULL, "surrogatepass");
  }
  if (PyBytes_Check(key))
  {
    Py_INCREF(key);
    return key;
  }
  if (UNLIKELY(PyBool_Check(key)))
  {
    return PyBytes_FromString(key == Py_True ? "true" : "false");
  }
  if (UNLIKELY(key == Py_None))
  {
    return PyBytes_FromString("null");
  }
  PyObject* keystr = PyObject_Str(key);
  if (!keystr)
  {
    PRINTMARK();
    return NULL;
  }
  key = PyUnicode_AsEncodedString(keystr, NULL, "surrogatepass");
  Py_DECREF(keystr);
  return key;
}

static int Dict_iterNext(PyObject *obj, TypeContext *tc)
{
  PyObject* key;
  if (!PyDict_Next(tc->dictObj, &tc->index, &key, &tc->itemValue))
  {
    PRINTMARK();
    return 0;
  }
  Py_XDECREF(tc->itemName);
  tc->itemName = Dict_convertKey(key);
  if (!tc->itemName)
  {
    return -1;
  }
  PRINTMARK();
  return 1;
}

static void Dict_iterEnd(PyObject *obj, TypeContext *tc)
{
  Py_CLEAR(tc->itemName);
  Py_DECREF(tc->dictObj);
  PRINTMARK();
}

static PyObject *Dict_iterGetValue(PyObject *obj, TypeContext *tc)
{
  return tc->itemValue;
}

static char *Dict_iterGetName(PyObject *obj, TypeContext *tc, size_t *outLen)
{
  *outLen = PyBytes_GET_SIZE(tc->itemName);
  return PyBytes_AS_STRING(tc->itemName);
}

static int SortedDict_iterNext(PyObject *obj, TypeContext *tc)
{
  // Upon first call, obtain a list of the keys and sort them. This follows the same logic as the
  // standard library's _json.c sort_keys handler.
  if (tc->newObj == NULL)
  {
    // Obtain the list of keys from the dictionary.
    PyObject *keys = PyDict_Keys(tc->dictObj);
    if (keys == NULL)
    {
      return -1;  // Out of memory
    }
    // Sort the list.
    if (PyList_Sort(keys) < 0)
    {
      Py_DECREF(keys);
      return -1;
    }
    // Store the sorted list of keys in the newObj slot.
    tc->newObj = keys;
    tc->size = PyList_GET_SIZE(keys);
  }

  if (tc->index >= tc->size)
  {
    PRINTMARK();
    return 0;
  }

  PyObject* key = PyList_GET_ITEM(tc->newObj, tc->index);
  Py_XDECREF(tc->itemName);
  tc->itemName = Dict_convertKey(key);
  if (!tc->itemName)
  {
    return -1;
  }
  tc->itemValue = PyDict_GetItem(tc->dictObj, key);
  if (!tc->itemValue)
  {
    return -1;  // Reachable only by concurrently deleting keys whilst serialising
  }
  tc->index++;
  return 1;
}

static void SetupDictIter(PyObject *dictObj, TypeContext *tc, JSONObjectEncoder *enc)
{
  tc->dictObj = dictObj;
  if (enc->sortKeys)
  {
    tc->iterNext = SortedDict_iterNext;
  }
  else
  {
    tc->iterNext = Dict_iterNext;
  }
  tc->iterEnd = Dict_iterEnd;
  tc->iterGetValue = Dict_iterGetValue;
  tc->iterGetName = Dict_iterGetName;
  tc->index = 0;
}

static void Object_beginTypeContext (PyObject *obj, TypeContext *tc, JSONObjectEncoder *enc)
{
  PyObject *objRepr, *newObj;
  int level = 0;
  PRINTMARK();
  if (!obj)  // Reachable only by making iterGetValue() fail (e.g. truncating a list mid-serialisation)
  {
    tc->type = JT_INVALID;
    return;
  }

  tc->newObj = NULL;
  tc->utf8BytesObj = NULL;
  tc->dictObj = NULL;
  tc->itemValue = NULL;
  tc->itemName = NULL;
  tc->index = 0;
  tc->size = 0;
  tc->longValue = 0;
  tc->rawJSONValue = NULL;

BEGIN:
  if (PyBool_Check(obj))
  {
    PRINTMARK();
    tc->type = (obj == Py_True) ? JT_TRUE : JT_FALSE;
    return;
  }
  else
  if (PyLong_Check(obj))
  {
    PRINTMARK();
    tc->PyTypeToJSON = PyLongToINT64;
    tc->type = JT_LONG;
    tc->longValue = PyLong_AsLongLong(obj);
    if (!(tc->longValue == -1 && PyErr_Occurred()))
    {
      return;
    }
    if (!PyErr_ExceptionMatches(PyExc_OverflowError))
    {
      goto INVALID;  // Probably out of memory
    }
    PyErr_Clear();
    tc->PyTypeToJSON = PyLongToUINT64;
    tc->type = JT_ULONG;
    tc->unsignedLongValue = PyLong_AsUnsignedLongLong(obj);
    if (!(tc->unsignedLongValue == (unsigned long long)-1 && PyErr_Occurred()))
    {
      return;
    }
    if (!PyErr_ExceptionMatches(PyExc_OverflowError))
    {
      goto INVALID;  // Probably out of memory
    }
    PyErr_Clear();
    tc->rawJSONValue = PyNumber_ToBase(obj, 10);
    if (!tc->rawJSONValue)
    {
      goto INVALID;
    }
    tc->PyTypeToJSON = PyLongToINTSTR;
    tc->type = JT_RAW;
    return;
  }
  else
  if (UNLIKELY(PyBytes_Check(obj) && !enc->rejectBytes))
  {
    tc->PyTypeToJSON = PyStringToUTF8; tc->type = JT_UTF8;
    return;
  }
  else
  if (PyUnicode_Check(obj))
  {
    PRINTMARK();
    tc->PyTypeToJSON = PyUnicodeToUTF8; tc->type = JT_UTF8;
    return;
  }
  else
  if (obj == Py_None)
  {
    PRINTMARK();
    tc->type = JT_NULL;
    return;
  }
  else
  if (PyFloat_Check(obj) || object_is_decimal_type(obj))
  {
    PRINTMARK();
    tc->PyTypeToJSON = PyFloatToDOUBLE; tc->type = JT_DOUBLE;
    return;
  }
  else
  if (PyDict_Check(obj))
  {
    PRINTMARK();
    tc->type = JT_OBJECT;
    SetupDictIter(obj, tc, enc);
    Py_INCREF(obj);
    return;
  }
  else
  if (PyList_Check(obj))
  {
    PRINTMARK();
    tc->type = JT_ARRAY;
    tc->iterEnd = List_iterEnd;
    tc->iterNext = List_iterNext;
    tc->iterGetValue = List_iterGetValue;
    tc->index =  0;
    tc->size = PyList_GET_SIZE( (PyObject *) obj);
    return;
  }
  else
  if (PyTuple_Check(obj))
  {
    PRINTMARK();
    tc->type = JT_ARRAY;
    tc->iterEnd = Tuple_iterEnd;
    tc->iterNext = Tuple_iterNext;
    tc->iterGetValue = Tuple_iterGetValue;
    tc->index = 0;
    tc->size = PyTuple_GET_SIZE( (PyObject *) obj);
    tc->itemValue = NULL;

    return;
  }

  if (UNLIKELY(PyObject_HasAttrString(obj, "toDict")))
  {
    PyObject* toDictResult = PyObject_CallMethod(obj, "toDict", NULL);
    if (toDictResult == NULL)
    {
      goto INVALID;
    }

    if (!PyDict_Check(toDictResult))
    {
      PyErr_Format(PyExc_TypeError, "toDict() should return a dict, got %s",
                   Py_TYPE(toDictResult)->tp_name);
      Py_DECREF(toDictResult);
      goto INVALID;
    }

    PRINTMARK();
    tc->type = JT_OBJECT;
    SetupDictIter(toDictResult, tc, enc);
    return;
  }
  else
  if (UNLIKELY(PyObject_HasAttrString(obj, "__json__")))
  {
    PyObject* toJSONResult = PyObject_CallMethod(obj, "__json__", NULL);
    if (toJSONResult == NULL)
    {
      goto INVALID;
    }

    if (!PyBytes_Check(toJSONResult) && !PyUnicode_Check(toJSONResult))
    {
      PyErr_Format(PyExc_TypeError, "__json__() should return str or bytes, got %s",
                   Py_TYPE(toJSONResult)->tp_name);
      Py_DECREF(toJSONResult);
      goto INVALID;
    }

    PRINTMARK();
    tc->PyTypeToJSON = PyRawJSONToUTF8;
    tc->type = JT_RAW;
    tc->rawJSONValue = toJSONResult;
    return;
  }

  if (enc->defaultFn)
  {
    // Break infinite loop
    if (level >= DEFAULT_FN_MAX_DEPTH)
    {
      PRINTMARK();
      PyErr_Format(PyExc_TypeError, "maximum recursion depth exceeded");
      goto INVALID;
    }

    newObj = PyObject_CallFunctionObjArgs(enc->defaultFn, obj, NULL);
    if (newObj)
    {
      PRINTMARK();
      Py_XDECREF(tc->newObj);
      obj = tc->newObj = newObj;
      level += 1;
      goto BEGIN;
    }
    else
    {
      goto INVALID;
    }
  }

  PRINTMARK();
  PyErr_Clear();

  objRepr = PyObject_Repr(obj);
  if (!objRepr)
  {
    goto INVALID;
  }
  PyObject* str = PyUnicode_AsEncodedString(objRepr, NULL, "strict");
  if (str)
  {
    PyErr_Format (PyExc_TypeError, "%s is not JSON serializable", PyBytes_AS_STRING(str));
  }
  Py_XDECREF(str);
  Py_DECREF(objRepr);

INVALID:
  PRINTMARK();
  tc->type = JT_INVALID;
  Py_XDECREF(tc->newObj);
  return;
}

static void Object_endTypeContext(PyObject *obj, TypeContext *tc)
{
  Py_XDECREF(tc->newObj);
  Py_XDECREF(tc->utf8BytesObj);

  if (tc->type == JT_RAW)
  {
    Py_XDECREF(tc->rawJSONValue);
  }
}

static const char *Object_getStringValue(PyObject *obj, TypeContext *tc, size_t *_outLen)
{
  obj = GET_OBJ(obj, tc);
  return tc->PyTypeToJSON (obj, tc, NULL, _outLen);
}

static int64_t Object_getLongValue(PyObject *obj, TypeContext *tc)
{
  int64_t ret;
  obj = GET_OBJ(obj, tc);
  tc->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static uint64_t Object_getUnsignedLongValue(PyObject *obj, TypeContext *tc)
{
  uint64_t ret;
  obj = GET_OBJ(obj, tc);
  tc->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static double Object_getDoubleValue(PyObject *obj, TypeContext *tc)
{
  double ret;
  obj = GET_OBJ(obj, tc);
  tc->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static int Object_iterNext(PyObject *obj, TypeContext *tc)
{
  obj = GET_OBJ(obj, tc);
  return tc->iterNext(obj, tc);
}

static void Object_iterEnd(PyObject *obj, TypeContext *tc)
{
  obj = GET_OBJ(obj, tc);
  tc->iterEnd(obj, tc);
}

static PyObject *Object_iterGetValue(PyObject *obj, TypeContext *tc)
{
  obj = GET_OBJ(obj, tc);
  return tc->iterGetValue(obj, tc);
}

static char *Object_iterGetName(PyObject *obj, TypeContext *tc, size_t *outLen)
{
  obj = GET_OBJ(obj, tc);
  return tc->iterGetName(obj, tc, outLen);
}

PyObject* ujson_dumps(PyObject* self, PyObject *args, PyObject *kwargs)
{
  static char *kwlist[] = { "obj", "ensure_ascii", "encode_html_chars", "escape_forward_slashes", "sort_keys", "indent", "allow_nan", "reject_bytes", "default", "separators", NULL };

  char buffer[65536];
  char *ret;
  const char *csNan = NULL, *csInf = NULL;
  PyObject *newobj;
  PyObject *oinput = NULL;
  int ensureAscii = true;
  int encodeHTMLChars = false;
  int escapeForwardSlashes = true;
  int sortKeys = false;
  PyObject *odefaultFn = NULL;
  PyObject *oseparators = NULL;
  PyObject *oseparatorsItem = NULL;
  PyObject *separatorsItemBytes = NULL;
  PyObject *oseparatorsKey = NULL;
  PyObject *separatorsKeyBytes = NULL;
  int allowNan = true;
  int rejectBytes = true;
  int indent = 0;
  size_t retLen = 0;

  JSONObjectEncoder encoder =
  {
    -1, //recursionMax
    true, //forceAscii
    false, //encodeHTMLChars
    true, //escapeForwardSlashes
    false, //sortKeys
    0, //indent
    true, //allowNan
    true, //rejectBytes
    0, //itemSeparatorLength
    NULL, //itemSeparatorChars
    0, //keySeparatorLength
    NULL, //keySeparatorChars
    NULL, //defaultFn
  };


  PRINTMARK();

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ppppippOO", kwlist, &oinput, &ensureAscii, &encodeHTMLChars, &escapeForwardSlashes, &sortKeys, &indent, &allowNan, &rejectBytes, &odefaultFn, &oseparators))
  {
    return NULL;
  }

  encoder.forceASCII = (bool) ensureAscii;
  encoder.encodeHTMLChars = (bool) encodeHTMLChars;
  encoder.escapeForwardSlashes = (bool) escapeForwardSlashes;
  encoder.sortKeys = (bool) sortKeys;
  encoder.allowNan = (bool) allowNan;
  encoder.rejectBytes = (bool) rejectBytes;

  if (odefaultFn != NULL && odefaultFn != Py_None)
  {
    encoder.defaultFn = odefaultFn;
  }

  if (encoder.allowNan)
  {
    csInf = "Infinity";
    csNan = "NaN";
  }

  if (indent < -1)
  {
    encoder.indent = -1;
  }
  else if (indent > 1000)
  {
    PyErr_SetString(PyExc_ValueError, "Maximum allowed indentation is 1000");
    return NULL;
  }
  else {
    encoder.indent = indent;
  }

  if (oseparators != NULL && oseparators != Py_None)
  {
    if (!PyTuple_Check(oseparators))
    {
      PyErr_SetString(PyExc_TypeError, "expected tuple or None as separator");
      return NULL;
    }
    if (PyTuple_GET_SIZE(oseparators) != 2)
    {
      PyErr_SetString(PyExc_ValueError, "expected tuple of size 2 as separator");
      return NULL;
    }
    oseparatorsItem = PyTuple_GET_ITEM(oseparators, 0);
    if (!PyUnicode_Check(oseparatorsItem))
    {
      PyErr_SetString(PyExc_TypeError, "expected str as item separator");
      return NULL;
    }
    oseparatorsKey = PyTuple_GET_ITEM(oseparators, 1);
    if (!PyUnicode_Check(oseparatorsKey))
    {
      PyErr_SetString(PyExc_TypeError, "expected str as key separator");
      return NULL;
    }
    encoder.itemSeparatorChars = PyUnicodeToUTF8Raw(oseparatorsItem, &encoder.itemSeparatorLength, &separatorsItemBytes);
    if (encoder.itemSeparatorChars == NULL)
    {
      goto ERROR;  // Out of memory
    }
    encoder.keySeparatorChars = PyUnicodeToUTF8Raw(oseparatorsKey, &encoder.keySeparatorLength, &separatorsKeyBytes);
    if (encoder.keySeparatorChars == NULL)
    {
      goto ERROR;  // Out of memory
    }
  }
  else
  {
    // Default to most compact representation
    encoder.itemSeparatorChars = ",";
    encoder.itemSeparatorLength = 1;
    if (encoder.indent)
    {
      // Extra space when indentation is in use
      encoder.keySeparatorChars = ": ";
      encoder.keySeparatorLength = 2;
    }
    else
    {
      encoder.keySeparatorChars = ":";
      encoder.keySeparatorLength = 1;
    }
  }

  encoder.d2s = NULL;
  dconv_d2s_init(&encoder.d2s, DCONV_D2S_EMIT_TRAILING_DECIMAL_POINT | DCONV_D2S_EMIT_TRAILING_ZERO_AFTER_POINT | DCONV_D2S_EMIT_POSITIVE_EXPONENT_SIGN,
                 csInf, csNan, 'e', DCONV_DECIMAL_IN_SHORTEST_LOW, DCONV_DECIMAL_IN_SHORTEST_HIGH, 0, 0);

  PRINTMARK();
  ret = JSON_EncodeObject (oinput, &encoder, buffer, sizeof (buffer), &retLen);
  PRINTMARK();

  dconv_d2s_free(&encoder.d2s);
  Py_XDECREF(separatorsItemBytes);
  Py_XDECREF(separatorsKeyBytes);

  if (encoder.errorMsg)
  {
    // If there is an error message and we don't already have a Python exception, set one.
    if (!PyErr_Occurred())
      PyErr_Format(PyExc_OverflowError, "%s", encoder.errorMsg);
    return NULL;
  }

  if (PyErr_Occurred())
  {
    if (ret != buffer)
    {
      PyObject_Free (ret);
    }

    return NULL;
  }

  newobj = PyUnicode_DecodeUTF8(ret, retLen, "surrogatepass");

  if (ret != buffer)
  {
    PyObject_Free (ret);
  }

  PRINTMARK();

  return newobj;

ERROR:  // Out of memory
  Py_XDECREF(separatorsItemBytes);
  Py_XDECREF(separatorsKeyBytes);
  return NULL;
}

PyObject* ujson_dump(PyObject* self, PyObject *args, PyObject *kwargs)
{
  PyObject *data;
  PyObject *file;
  PyObject *string;
  PyObject *write;
  PyObject *argtuple;
  PyObject *write_result;

  PRINTMARK();

  if (!PyArg_ParseTuple (args, "OO", &data, &file))
  {
    return NULL;
  }

  if (!PyObject_HasAttrString (file, "write"))
  {
    PyErr_Format (PyExc_TypeError, "expected file");
    return NULL;
  }

  write = PyObject_GetAttrString (file, "write");

  if (!PyCallable_Check (write))
  {
    Py_XDECREF(write);
    PyErr_Format (PyExc_TypeError, "expected file");
    return NULL;
  }

  argtuple = PyTuple_Pack(1, data);
  if (argtuple == NULL)  // Out of memory
  {
    Py_XDECREF(write);
    return NULL;
  }

  string = ujson_dumps (self, argtuple, kwargs);

  if (string == NULL)
  {
    Py_XDECREF(write);
    Py_XDECREF(argtuple);
    return NULL;
  }

  Py_XDECREF(argtuple);

  argtuple = PyTuple_Pack (1, string);
  if (argtuple == NULL)  // Out of memory
  {
    Py_XDECREF(write);
    Py_DECREF(string);
    return NULL;
  }

  write_result = PyObject_CallObject (write, argtuple);
  if (write_result == NULL)
  {
    Py_XDECREF(write);
    Py_DECREF(string);
    Py_XDECREF(argtuple);
    return NULL;
  }

  Py_DECREF(write_result);
  Py_XDECREF(write);
  Py_DECREF(argtuple);
  Py_XDECREF(string);

  PRINTMARK();

  Py_RETURN_NONE;
}

/*
FIXME: While this is fine dandy and working it's a magic value mess which probably only the author understands.
Needs a cleanup and more documentation */

/*
Table for pure ascii output escaping all characters above 127 to \uXXXX */
static const uint8_t g_asciiOutputTable[256] =
{
/* 0x00 */ 0, 30, 30, 30, 30, 30, 30, 30, 10, 12, 14, 30, 16, 18, 30, 30,
/* 0x10 */ 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
/* 0x20 */ 1, 1, 20, 1, 1, 1, 29, 1, 1, 1, 1, 1, 1, 1, 1, 24,
/* 0x30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 29, 1, 29, 1,
/* 0x40 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x50 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 22, 1, 1, 1,
/* 0x60 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x70 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x80 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x90 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0xa0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0xb0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0xc0 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* 0xd0 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* 0xe0 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
/* 0xf0 */ 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

static void SetError (PyObject *obj, JSONObjectEncoder *enc, const char *message)
{
  enc->errorMsg = message;
  enc->errorObj = obj;
}

/*
FIXME: Keep track of how big these get across several encoder calls and try to make an estimate
That way we won't run our head into the wall each call */
static void Buffer_Realloc (JSONObjectEncoder *enc, size_t cbNeeded)
{
  size_t free_space = enc->end - enc->offset;
  if (free_space >= cbNeeded)
  {
    return;
  }
  size_t curSize = enc->end - enc->start;
  size_t newSize = curSize;
  size_t offset = enc->offset - enc->start;

#ifdef DEBUG
  // In debug mode, allocate only what is requested so that any miscalculation
  // shows up plainly as a crash.
  newSize = (enc->offset - enc->start) + cbNeeded;
#else
  while (newSize < curSize + cbNeeded)
  {
    newSize *= 2;
  }
#endif

  if (enc->heap)
  {
    enc->start = (char *) PyObject_Realloc (enc->start, newSize);
    if (!enc->start)
    {
      SetError (NULL, enc, "Could not reserve memory block");
      return;
    }
  }
  else
  {
    char *oldStart = enc->start;
    enc->heap = true;
    enc->start = (char *) PyObject_Malloc (newSize);
    if (!enc->start)
    {
      SetError (NULL, enc, "Could not reserve memory block");
      return;
    }
    memcpy (enc->start, oldStart, offset);
  }
  enc->offset = enc->start + offset;
  enc->end = enc->start + newSize;
}

#define Buffer_Reserve(__enc, __len) \
    if ( (size_t) ((__enc)->end - (__enc)->offset) < (size_t) (__len))  \
    {   \
      Buffer_Realloc((__enc), (__len));\
    }   \

static void *Buffer_memcpy (JSONObjectEncoder *enc, const void *src, size_t n)
{
  void *out;
#ifdef DEBUG
  if ((size_t) (enc->end - enc->offset) < n) {
    fprintf(stderr, "Ran out of buffer space during Buffer_memcpy()\n");
    abort();
  }
#endif
  out = memcpy(enc->offset, src, n);
  enc->offset += n;
  return out;
}

static FASTCALL_ATTR INLINE_PREFIX void FASTCALL_MSVC Buffer_AppendShortHexUnchecked (char *outputOffset, unsigned short value)
{
  *(outputOffset++) = g_hexChars[(value & 0xf000) >> 12];
  *(outputOffset++) = g_hexChars[(value & 0x0f00) >> 8];
  *(outputOffset++) = g_hexChars[(value & 0x00f0) >> 4];
  *(outputOffset++) = g_hexChars[(value & 0x000f) >> 0];
}

static void Buffer_EscapeStringUnvalidated (JSONObjectEncoder *enc, const char *io, const char *end)
{
  char *of = (char *) enc->offset;

  for (;;)
  {
#ifdef DEBUG
    // 6 is the maximum length of a single character (cf. RESERVE_STRING).
    if ((io < end) && (enc->end - of < 6)) {
      fprintf(stderr, "Ran out of buffer space during Buffer_EscapeStringUnvalidated()\n");
      abort();
    }
#endif
    switch (*io)
    {
      case 0x00:
      {
        if (io < end)
        {
          *(of++) = '\\';
          *(of++) = 'u';
          *(of++) = '0';
          *(of++) = '0';
          *(of++) = '0';
          *(of++) = '0';
          break;
        }
        else
        {
          enc->offset += (of - enc->offset);
          return;
        }
      }
      case '\"': (*of++) = '\\'; (*of++) = '\"'; break;
      case '\\': (*of++) = '\\'; (*of++) = '\\'; break;
      case '\b': (*of++) = '\\'; (*of++) = 'b'; break;
      case '\f': (*of++) = '\\'; (*of++) = 'f'; break;
      case '\n': (*of++) = '\\'; (*of++) = 'n'; break;
      case '\r': (*of++) = '\\'; (*of++) = 'r'; break;
      case '\t': (*of++) = '\\'; (*of++) = 't'; break;

      case '/':
      {
        if (enc->escapeForwardSlashes)
        {
          (*of++) = '\\';
          (*of++) = '/';
        }
        else
        {
          // Same as default case below.
          (*of++) = (*io);
        }
        break;
      }
      case 0x26: // '&'
      case 0x3c: // '<'
      case 0x3e: // '>'
      {
        if (enc->encodeHTMLChars)
        {
          // Fall through to \u00XX case below.
        }
        else
        {
          // Same as default case below.
          (*of++) = (*io);
          break;
        }
      }
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
      case 0x0b:
      case 0x0e:
      case 0x0f:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
      {
        *(of++) = '\\';
        *(of++) = 'u';
        *(of++) = '0';
        *(of++) = '0';
        *(of++) = g_hexChars[ (unsigned char) (((*io) & 0xf0) >> 4)];
        *(of++) = g_hexChars[ (unsigned char) ((*io) & 0x0f)];
        break;
      }
      default: (*of++) = (*io); break;
    }
    io++;
  }
}

static bool Buffer_EscapeStringValidated (PyObject *obj, JSONObjectEncoder *enc, const char *io, const char *end)
{
  uint32_t ucs;
  char *of = (char *) enc->offset;

  for (;;)
  {
#ifdef DEBUG
    /*
    6 is the maximum length of a single character (cf. RESERVE_STRING).
    Note that the loop below may consume more than one input char and produce a UTF-16 surrogate pair.
    In that case, more than 6 characters would be needed on the output buffer.
    So this calculates the maximum length of the entire remaining input buffer instead. */
    if (enc->end - enc->offset < 6 * (end - io)) {
      fprintf(stderr, "Ran out of buffer space during Buffer_EscapeStringValidated()\n");
      abort();
    }
#endif
    uint8_t utflen = g_asciiOutputTable[(unsigned char) *io];

    switch (utflen)
    {
      case 0:
      {
        if (io < end)
        {
          *(of++) = '\\';
          *(of++) = 'u';
          *(of++) = '0';
          *(of++) = '0';
          *(of++) = '0';
          *(of++) = '0';
          io ++;
          continue;
        }
        else
        {
          enc->offset += (of - enc->offset);
          return true;
        }
      }

      case 1:
      {
        *(of++)= (*io++);
        continue;
      }

      // https://en.wikipedia.org/wiki/UTF-8#Description
      case 2:
      {
        uint32_t in;
        uint16_t in16;

        if (end - io < 2)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Unterminated UTF-8 sequence when encoding string");
          return false;
        }
        if ((io[1] & 0xc0) != 0x80)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Invalid continuation byte in 2-byte UTF-8 sequence detected when encoding string");
          return false;
        }

        memcpy(&in16, io, sizeof(uint16_t));
        in = (uint32_t) in16;

#ifdef __LITTLE_ENDIAN__
        ucs = ((in & 0x1f) << 6) | ((in >> 8) & 0x3f);
#else
        ucs = ((in & 0x1f00) >> 2) | (in & 0x3f);
#endif

        if (ucs < 0x80)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Overlong 2-byte UTF-8 sequence detected when encoding string");
          return false;
        }

        io += 2;
        break;
      }

      case 3:
      {
        uint32_t in;
        uint16_t in16;
        uint8_t in8;

        if (end - io < 3)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Unterminated UTF-8 sequence when encoding string");
          return false;
        }
        if ((io[1] & 0xc0) != 0x80 || (io[2] & 0xc0) != 0x80)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Invalid continuation byte in 3-byte UTF-8 sequence detected when encoding string");
          return false;
        }
        // Under normal UTF-8 decoding rules, UTF-16 surrogates should also be disallowed
        // but in JSON, they're special cased and rewritten later as \udc7f.
        // if ((uint8_t) io[0] == 0xed && (uint8_t) io[1] >= 0xa0)
        // {
        //   enc->offset += (of - enc->offset);
        //   SetError (obj, enc, "Illegal UTF-16 surrogate in 3-byte UTF-8 sequence detected when encoding string");
        //   return false;
        // }
        memcpy(&in16, io, sizeof(uint16_t));
        memcpy(&in8, io + 2, sizeof(uint8_t));
#ifdef __LITTLE_ENDIAN__
        in = (uint32_t) in16;
        in |= in8 << 16;
        ucs = ((in & 0x0f) << 12) | ((in & 0x3f00) >> 2) | ((in & 0x3f0000) >> 16);
#else
        in = in16 << 8;
        in |= in8;
        ucs = ((in & 0x0f0000) >> 4) | ((in & 0x3f00) >> 2) | (in & 0x3f);
#endif

        if (ucs < 0x800)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Overlong 3-byte UTF-8 sequence detected when encoding string");
          return false;
        }

        io += 3;
        break;
      }
      case 4:
      {
        uint32_t in;

        if (end - io < 4)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Unterminated UTF-8 sequence when encoding string");
          return false;
        }
        if ((io[1] & 0xc0) != 0x80 || (io[2] & 0xc0) != 0x80 || (io[3] & 0xc0) != 0x80)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Invalid continuation byte in 4-byte UTF-8 sequence detected when encoding string");
          return false;
        }
        if (((uint8_t) io[0] >= 0xf4 && (uint8_t) io[1] >= 0x90) || (uint8_t) io[0] >= 0xf5)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, ">U+10FFFF in 4-byte UTF-8 sequence detected when encoding string");
          return false;
        }

        memcpy(&in, io, sizeof(uint32_t));
#ifdef __LITTLE_ENDIAN__
        ucs = ((in & 0x07) << 18) | ((in & 0x3f00) << 4) | ((in & 0x3f0000) >> 10) | ((in & 0x3f000000) >> 24);
#else
        ucs = ((in & 0x07000000) >> 6) | ((in & 0x3f0000) >> 4) | ((in & 0x3f00) >> 2) | (in & 0x3f);
#endif
        if (ucs < 0x10000)
        {
          enc->offset += (of - enc->offset);
          SetError (obj, enc, "Overlong 4-byte UTF-8 sequence detected when encoding string");
          return false;
        }

        io += 4;
        break;
      }


      case 5:
      case 6:
      {
        enc->offset += (of - enc->offset);
        SetError (obj, enc, "Unsupported UTF-8 sequence length when encoding string");
        return false;
      }

      case 29:
      {
        if (enc->encodeHTMLChars)
        {
          // Fall through to \u00XX case 30 below.
        }
        else
        {
          // Same as case 1 above.
          *(of++) = (*io++);
          continue;
        }
      }

      case 30:
      {
        // \uXXXX encode
        *(of++) = '\\';
        *(of++) = 'u';
        *(of++) = '0';
        *(of++) = '0';
        *(of++) = g_hexChars[ (unsigned char) (((*io) & 0xf0) >> 4)];
        *(of++) = g_hexChars[ (unsigned char) ((*io) & 0x0f)];
        io ++;
        continue;
      }
      case 10:
      case 12:
      case 14:
      case 16:
      case 18:
      case 20:
      case 22:
      {
        *(of++) = *( (char *) (g_escapeChars + utflen + 0));
        *(of++) = *( (char *) (g_escapeChars + utflen + 1));
        io ++;
        continue;
      }
      case 24:
      {
        if (enc->escapeForwardSlashes)
        {
          *(of++) = *( (char *) (g_escapeChars + utflen + 0));
          *(of++) = *( (char *) (g_escapeChars + utflen + 1));
          io ++;
        }
        else
        {
          // Same as case 1 above.
          *(of++) = (*io++);
        }
        continue;
      }
      // This can never happen, it's here to make L4 VC++ happy
      default:
      {
        ucs = 0;
        break;
      }
    }

    /*
    If the character is a UTF8 sequence of length > 1 we end up here */
    if (ucs >= 0x10000)
    {
      ucs -= 0x10000;
      *(of++) = '\\';
      *(of++) = 'u';
      Buffer_AppendShortHexUnchecked(of, (unsigned short) (ucs >> 10) + 0xd800);
      of += 4;

      *(of++) = '\\';
      *(of++) = 'u';
      Buffer_AppendShortHexUnchecked(of, (unsigned short) (ucs & 0x3ff) + 0xdc00);
      of += 4;
    }
    else
    {
      *(of++) = '\\';
      *(of++) = 'u';
      Buffer_AppendShortHexUnchecked(of, (unsigned short) ucs);
      of += 4;
    }
  }
}


static FASTCALL_ATTR INLINE_PREFIX void FASTCALL_MSVC Buffer_AppendCharUnchecked(JSONObjectEncoder *enc, char chr)
{
#ifdef DEBUG
  if (enc->end <= enc->offset)
  {
    fprintf(stderr, "Overflow writing byte %d '%c'. The last few characters were:\n'''", chr, chr);
    char * recent = enc->offset - 1000;
    if (enc->start > recent)
    {
      recent = enc->start;
    }
    for (; recent < enc->offset; recent++)
    {
      fprintf(stderr, "%c", *recent);
    }
    fprintf(stderr, "'''\n");
    abort();
  }
#endif
  *(enc->offset++) = chr;
}

static FASTCALL_ATTR INLINE_PREFIX void FASTCALL_MSVC strreverse(char* begin, char* end)
{
  char aux;
  while (end > begin)
  aux = *end, *end-- = *begin, *begin++ = aux;
}

static void Buffer_AppendIndentNewlineUnchecked(JSONObjectEncoder *enc)
{
  if (enc->indent > 0) Buffer_AppendCharUnchecked(enc, '\n');
}

static void Buffer_AppendIndentUnchecked(JSONObjectEncoder *enc, int32_t value)
{
  ptrdiff_t i;
  if (enc->indent > 0)
    while (value-- > 0)
      for (i = 0; i < enc->indent; i++)
        Buffer_AppendCharUnchecked(enc, ' ');
}

static void Buffer_AppendLongUnchecked(JSONObjectEncoder *enc, int64_t value)
{
  char* wstr;
  uint64_t uvalue;

  if (value == INT64_MIN) {
    uvalue = INT64_MAX + UINT64_C(1);
  } else {
    uvalue = (value < 0) ? -value : value;
  }

  wstr = enc->offset;
#ifdef DEBUG
  // 20 is the maximum length of a int64_t (minus sign plus 19 digits)
  if (enc->end - enc->offset < 20) {
    fprintf(stderr, "Ran out of buffer space during Buffer_AppendLongUnchecked()\n");
    abort();
  }
#endif
  // Conversion. Number is reversed.

  do *wstr++ = (char)(48 + (uvalue % 10ULL)); while(uvalue /= 10ULL);
  if (value < 0) *wstr++ = '-';

  // Reverse string
  strreverse(enc->offset,wstr - 1);
  enc->offset += (wstr - (enc->offset));
}

static void Buffer_AppendUnsignedLongUnchecked(JSONObjectEncoder *enc, uint64_t value)
{
  char* wstr;
  uint64_t uvalue = value;

  wstr = enc->offset;
#ifdef DEBUG
  // 21 is the maximum length of a uint64_t (minus sign plus 20 digits)
  if (enc->end - enc->offset < 21) {
    fprintf(stderr, "Ran out of buffer space during Buffer_AppendUnsignedLongUnchecked()\n");
    abort();
  }
#endif
  // Conversion. Number is reversed.

  do *wstr++ = (char)(48 + (uvalue % 10ULL)); while(uvalue /= 10ULL);

  // Reverse string
  strreverse(enc->offset,wstr - 1);
  enc->offset += (wstr - (enc->offset));
}

static bool Buffer_AppendDoubleDconv(PyObject *obj, JSONObjectEncoder *enc, double value)
{
  char buf[128];
  int strlength;
#ifdef DEBUG
  if ((size_t) (enc->end - enc->offset) < sizeof(buf)) {
    fprintf(stderr, "Ran out of buffer space during Buffer_AppendDoubleDconv()\n");
    abort();
  }
#endif
  if(!dconv_d2s(enc->d2s, value, buf, sizeof(buf), &strlength))
  {
    SetError (obj, enc, "Invalid value when encoding double");
    return false;
  }

  Buffer_memcpy(enc, buf, strlength);

  return true;
}

/*
FIXME:
Handle integration functions returning NULL here */

/*
FIXME:
Perhaps implement recursion detection */

static void encode(PyObject *obj, JSONObjectEncoder *enc, const char *name, size_t cbName)
{
  const char *value;
  char *objName;
  int count, res;
  PyObject *iterObj;
  size_t szlen;
  TypeContext tc;

  if (enc->level > enc->recursionMax)
  {
    SetError (obj, enc, "Maximum recursion level reached");
    return;
  }

  if (enc->errorMsg)  // Out of memory
  {
    return;
  }

  if (name)
  {
    Buffer_Reserve(enc, RESERVE_STRING(cbName) + enc->keySeparatorLength);
    Buffer_AppendCharUnchecked(enc, '\"');

    if (enc->forceASCII)
    {
      if (!Buffer_EscapeStringValidated(obj, enc, name, name + cbName))
      {
        return;
      }
    }
    else
    {
      Buffer_EscapeStringUnvalidated(enc, name, name + cbName);
    }

    Buffer_AppendCharUnchecked(enc, '\"');

    Buffer_memcpy(enc, enc->keySeparatorChars, enc->keySeparatorLength);
  }

  Object_beginTypeContext(obj, &tc, enc);

  /*
  This reservation covers any additions on non-variable parts below, specifically:
  - Opening brackets for JT_ARRAY and JT_OBJECT
  - Number representation for JT_LONG, JT_ULONG, JT_INT, and JT_DOUBLE
  - Constant value for JT_TRUE, JT_FALSE, JT_NULL

  The length of 128 is the worst case length of the Buffer_AppendDoubleDconv addition.
  The other types above all have smaller representations.
  */
  Buffer_Reserve (enc, 128);

  switch (tc.type)
  {
    case JT_INVALID:
    {
      /*
      There should already be an exception at the Python level.
      This however sets the errorMsg so recursion on arrays and objects stops.
      endTypeContext must not be called here as beginTypeContext already cleans up in the INVALID case.
      */
      SetError (obj, enc, "Invalid type");
      enc->level--;
      return;
    }

    case JT_ARRAY:
    {
      count = 0;

      Buffer_AppendCharUnchecked (enc, '[');

      // The extra 1 byte covers the optional newline.
      size_t per_item_reserve = (enc->indent > 0 ? enc->indent : 0) * (enc->level + 1) + enc->itemSeparatorLength + 1;
      while (Object_iterNext(obj, &tc))
      {
        Buffer_Reserve (enc, per_item_reserve);

        if (count > 0)
        {
          Buffer_memcpy(enc, enc->itemSeparatorChars, enc->itemSeparatorLength);
        }
        Buffer_AppendIndentNewlineUnchecked (enc);

        iterObj = Object_iterGetValue(obj, &tc);

        enc->level ++;
        Buffer_AppendIndentUnchecked (enc, enc->level);
        encode (iterObj, enc, NULL, 0);
        if (enc->errorMsg)
        {
          Object_iterEnd(obj, &tc);
          Object_endTypeContext(obj, &tc);
          enc->level--;
          return;
        }
        count ++;
      }

      Object_iterEnd(obj, &tc);

      if (count > 0 && enc->indent > 0) {
        // Reserve space for the indentation plus the newline.
        Buffer_Reserve (enc, enc->indent * enc->level + 1);
        Buffer_AppendIndentNewlineUnchecked (enc);
        Buffer_AppendIndentUnchecked (enc, enc->level);
      }
      Buffer_Reserve (enc, 1);
      Buffer_AppendCharUnchecked (enc, ']');
      break;
    }

    case JT_OBJECT:
    {
      count = 0;

      Buffer_AppendCharUnchecked (enc, '{');

      // The extra 1 byte covers the optional newline.
      size_t reserve_size = (enc->indent > 0 ? enc->indent : 0) * (enc->level + 1) + enc->itemSeparatorLength + 1;
      while ((res = Object_iterNext(obj, &tc)))
      {
        Buffer_Reserve (enc, reserve_size);

        if(res < 0)
        {
          Object_iterEnd(obj, &tc);
          Object_endTypeContext(obj, &tc);
          enc->level--;
          return;
        }

        if (count > 0)
        {
          Buffer_memcpy(enc, enc->itemSeparatorChars, enc->itemSeparatorLength);
        }
        Buffer_AppendIndentNewlineUnchecked (enc);

        iterObj = Object_iterGetValue(obj, &tc);
        objName = Object_iterGetName(obj, &tc, &szlen);

        enc->level ++;
        Buffer_AppendIndentUnchecked (enc, enc->level);
        encode (iterObj, enc, objName, szlen);
        if (enc->errorMsg)
        {
          Object_iterEnd(obj, &tc);
          Object_endTypeContext(obj, &tc);
          enc->level--;
          return;
        }
        count ++;
      }

      Object_iterEnd(obj, &tc);

      if (count > 0 && enc->indent > 0) {
        Buffer_Reserve (enc, enc->indent * enc->level + 1);
        Buffer_AppendIndentNewlineUnchecked (enc);
        Buffer_AppendIndentUnchecked (enc, enc->level);
      }
      Buffer_Reserve (enc, 1);
      Buffer_AppendCharUnchecked (enc, '}');
      break;
    }

    case JT_LONG:
    {
      Buffer_AppendLongUnchecked (enc, Object_getLongValue(obj, &tc));
      break;
    }

    case JT_ULONG:
    {
      Buffer_AppendUnsignedLongUnchecked (enc, Object_getUnsignedLongValue(obj, &tc));
      break;
    }

    case JT_TRUE:
    {
      Buffer_AppendCharUnchecked (enc, 't');
      Buffer_AppendCharUnchecked (enc, 'r');
      Buffer_AppendCharUnchecked (enc, 'u');
      Buffer_AppendCharUnchecked (enc, 'e');
      break;
    }

    case JT_FALSE:
    {
      Buffer_AppendCharUnchecked (enc, 'f');
      Buffer_AppendCharUnchecked (enc, 'a');
      Buffer_AppendCharUnchecked (enc, 'l');
      Buffer_AppendCharUnchecked (enc, 's');
      Buffer_AppendCharUnchecked (enc, 'e');
      break;
    }

    case JT_NULL:
    {
      Buffer_AppendCharUnchecked (enc, 'n');
      Buffer_AppendCharUnchecked (enc, 'u');
      Buffer_AppendCharUnchecked (enc, 'l');
      Buffer_AppendCharUnchecked (enc, 'l');
      break;
    }

    case JT_DOUBLE:
    {
      if (!Buffer_AppendDoubleDconv(obj, enc, Object_getDoubleValue(obj, &tc)))
      {
        Object_endTypeContext(obj, &tc);
        enc->level--;
        return;
      }
      break;
    }

    case JT_UTF8:
    {
      value = Object_getStringValue(obj, &tc, &szlen);
      if (!value)
      {
        Object_endTypeContext(obj, &tc);
        return;  // Out of memory
      }

      Buffer_Reserve(enc, RESERVE_STRING(szlen));
      if (enc->errorMsg)  // Out of memory (via Buffer_Reserve())
      {
        Object_endTypeContext(obj, &tc);
        return;
      }
      Buffer_AppendCharUnchecked (enc, '\"');

      if (enc->forceASCII)
      {
        if (!Buffer_EscapeStringValidated(obj, enc, value, value + szlen))
        {
          Object_endTypeContext(obj, &tc);
          enc->level--;
          return;
        }
      }
      else
      {
        Buffer_EscapeStringUnvalidated(enc, value, value + szlen);
      }

      Buffer_AppendCharUnchecked (enc, '\"');
      break;
    }

    case JT_RAW:
    {
      value = Object_getStringValue(obj, &tc, &szlen);
      if (!value)  // Out of memory
      {
        Object_endTypeContext(obj, &tc);
        return;
      }

      Buffer_Reserve(enc, szlen);
      if (enc->errorMsg)  // Out of memory (via Buffer_Reserve())
      {
        Object_endTypeContext(obj, &tc);
        return;
      }

      Buffer_memcpy(enc, value, szlen);

      break;
    }
  }

  Object_endTypeContext(obj, &tc);
  enc->level--;
}

static char *JSON_EncodeObject(PyObject *obj, JSONObjectEncoder *enc, char *_buffer, size_t _cbBuffer, size_t *_outLen)
{
  enc->errorMsg = NULL;
  enc->errorObj = NULL;
  enc->level = 0;

  if (enc->recursionMax < 1)
  {
    enc->recursionMax = JSON_MAX_RECURSION_DEPTH;
  }
  enc->start = _buffer;
  enc->heap = false;

  enc->end = enc->start + _cbBuffer;
  enc->offset = enc->start;

  encode (obj, enc, NULL, 0);

  if (enc->errorMsg)
  {
    if (enc->heap)
    {
      // Buffer was realloc'd at some point, or no initial buffer was provided.
      PyObject_Free(enc->start);
    }
    return NULL;
  }

  *_outLen = enc->offset - enc->start;
  return enc->start;
}
