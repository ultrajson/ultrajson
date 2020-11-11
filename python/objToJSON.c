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

#include <Python.h>
#include <stdio.h>
#include <ultrajson.h>

#define EPOCH_ORD 719163

typedef void *(*PFN_PyTypeToJSON)(JSOBJ obj, JSONTypeContext *ti, void *outValue, size_t *_outLen);

int object_is_decimal_type(PyObject *obj);

typedef struct __TypeContext
{
  JSPFN_ITEREND iterEnd;
  JSPFN_ITERNEXT iterNext;
  JSPFN_ITERGETNAME iterGetName;
  JSPFN_ITERGETVALUE iterGetValue;
  PFN_PyTypeToJSON PyTypeToJSON;
  PyObject *newObj;
  PyObject *dictObj;
  Py_ssize_t index;
  Py_ssize_t size;
  PyObject *itemValue;
  PyObject *itemName;
  PyObject *attrList;
  PyObject *iterator;

  union
  {
    PyObject *rawJSONValue;
    JSINT64 longValue;
    JSUINT64 unsignedLongValue;
  };
} TypeContext;

#define GET_TC(__ptrtc) ((TypeContext *)((__ptrtc)->prv))

struct PyDictIterState
{
  PyObject *keys;
  size_t i;
  size_t sz;
};

//#define PRINTMARK() fprintf(stderr, "%s: MARK(%d)\n", __FILE__, __LINE__)
#define PRINTMARK()

#ifdef _LP64
static void *PyIntToINT64(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = (PyObject *) _obj;
  *((JSINT64 *) outValue) = PyLong_AsLong (obj);
  return NULL;
}
#else
static void *PyIntToINT32(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = (PyObject *) _obj;
  *((JSINT32 *) outValue) = PyLong_AsLong (obj);
  return NULL;
}
#endif

static void *PyLongToINT64(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  *((JSINT64 *) outValue) = GET_TC(tc)->longValue;
  return NULL;
}

static void *PyLongToUINT64(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  *((JSUINT64 *) outValue) = GET_TC(tc)->unsignedLongValue;
  return NULL;
}

static void *PyFloatToDOUBLE(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = (PyObject *) _obj;
  *((double *) outValue) = PyFloat_AsDouble (obj);
  return NULL;
}

static void *PyStringToUTF8(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = (PyObject *) _obj;
  *_outLen = PyBytes_Size(obj);
  return PyBytes_AsString(obj);
}

static void *PyUnicodeToUTF8(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = (PyObject *) _obj;
  PyObject *newObj;
#ifndef Py_LIMITED_API
  if (PyUnicode_IS_COMPACT_ASCII(obj))
  {
    Py_ssize_t len;
    char *data = PyUnicode_AsUTF8AndSize(obj, &len);
    *_outLen = len;
    return data;
  }
#endif
  newObj = PyUnicode_AsUTF8String(obj);
  if(!newObj)
  {
    return NULL;
  }

  GET_TC(tc)->newObj = newObj;

  *_outLen = PyBytes_Size(newObj);
  return PyBytes_AsString(newObj);
}

static void *PyRawJSONToUTF8(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = GET_TC(tc)->rawJSONValue;
  if (PyUnicode_Check(obj))
  {
    return PyUnicodeToUTF8(obj, tc, outValue, _outLen);
  }
  else
  {
    return PyStringToUTF8(obj, tc, outValue, _outLen);
  }
}

static int Tuple_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
  PyObject *item;

  if (GET_TC(tc)->index >= GET_TC(tc)->size)
  {
    return 0;
  }

  item = PyTuple_GetItem (obj, GET_TC(tc)->index);

  GET_TC(tc)->itemValue = item;
  GET_TC(tc)->index ++;
  return 1;
}

static void Tuple_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
}

static JSOBJ Tuple_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
  return GET_TC(tc)->itemValue;
}

static char *Tuple_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
  return NULL;
}

static int List_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
  if (GET_TC(tc)->index >= GET_TC(tc)->size)
  {
    PRINTMARK();
    return 0;
  }

  GET_TC(tc)->itemValue = PyList_GetItem (obj, GET_TC(tc)->index);
  GET_TC(tc)->index ++;
  return 1;
}

static void List_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
}

static JSOBJ List_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
  return GET_TC(tc)->itemValue;
}

static char *List_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
  return NULL;
}

//=============================================================================
// Dict iteration functions
// itemName might converted to string (Python_Str). Do refCounting
// itemValue is borrowed from object (which is dict). No refCounting
//=============================================================================

static int Dict_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
  PyObject* itemNameTmp;

  if (GET_TC(tc)->itemName)
  {
    Py_DECREF(GET_TC(tc)->itemName);
    GET_TC(tc)->itemName = NULL;
  }

  if (!(GET_TC(tc)->itemName = PyIter_Next(GET_TC(tc)->iterator)))
  {
    PRINTMARK();
    return 0;
  }

  if (!(GET_TC(tc)->itemValue = PyDict_GetItem(GET_TC(tc)->dictObj, GET_TC(tc)->itemName)))
  {
    PRINTMARK();
    return 0;
  }

  if (PyUnicode_Check(GET_TC(tc)->itemName))
  {
    itemNameTmp = GET_TC(tc)->itemName;
    GET_TC(tc)->itemName = PyUnicode_AsUTF8String (GET_TC(tc)->itemName);
    Py_DECREF(itemNameTmp);
  }
  else
  if (!PyBytes_Check(GET_TC(tc)->itemName))
  {
    if (UNLIKELY(GET_TC(tc)->itemName == Py_None))
    {
      itemNameTmp = PyUnicode_FromString("null");
      GET_TC(tc)->itemName = PyUnicode_AsUTF8String(itemNameTmp);
      Py_DECREF(Py_None);
      return 1;
    }

    GET_TC(tc)->itemName = PyObject_Str(GET_TC(tc)->itemName);
    itemNameTmp = GET_TC(tc)->itemName;
    GET_TC(tc)->itemName = PyUnicode_AsUTF8String (GET_TC(tc)->itemName);
    Py_DECREF(itemNameTmp);
  }
  else
  {
    Py_INCREF(GET_TC(tc)->itemName);
  }
  PRINTMARK();
  return 1;
}

static void Dict_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
  if (GET_TC(tc)->itemName)
  {
    Py_DECREF(GET_TC(tc)->itemName);
    GET_TC(tc)->itemName = NULL;
  }
  Py_CLEAR(GET_TC(tc)->iterator);
  Py_DECREF(GET_TC(tc)->dictObj);
  PRINTMARK();
}

static JSOBJ Dict_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
  return GET_TC(tc)->itemValue;
}

static char *Dict_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
  *outLen = PyBytes_Size(GET_TC(tc)->itemName);
  return PyBytes_AsString(GET_TC(tc)->itemName);
}

static int SortedDict_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
  PyObject *items = NULL, *item = NULL, *key = NULL, *value = NULL;
  Py_ssize_t i, nitems;
  PyObject* keyTmp;

  // Upon first call, obtain a list of the keys and sort them. This follows the same logic as the
  // stanard library's _json.c sort_keys handler.
  if (GET_TC(tc)->newObj == NULL)
  {
    // Obtain the list of keys from the dictionary.
    items = PyMapping_Keys(GET_TC(tc)->dictObj);
    if (items == NULL)
    {
      goto error;
    }
    else if (!PyList_Check(items))
    {
      PyErr_SetString(PyExc_ValueError, "keys must return list");
      goto error;
    }

    // Sort the list.
    if (PyList_Sort(items) < 0)
    {
      PyErr_SetString(PyExc_ValueError, "unorderable keys");
      goto error;
    }

    // Obtain the value for each key, and pack a list of (key, value) 2-tuples.
    nitems = PyList_Size(items);
    for (i = 0; i < nitems; i++)
    {
      key = PyList_GetItem(items, i);
      value = PyDict_GetItem(GET_TC(tc)->dictObj, key);

      // Subject the key to the same type restrictions and conversions as in Dict_iterGetValue.
      if (PyUnicode_Check(key))
      {
        key = PyUnicode_AsUTF8String(key);
      }
      else if (!PyBytes_Check(key))
      {
        key = PyObject_Str(key);
        keyTmp = key;
        key = PyUnicode_AsUTF8String(key);
        Py_DECREF(keyTmp);
      }
      else
      {
        Py_INCREF(key);
      }

      item = PyTuple_Pack(2, key, value);
      if (item == NULL)
      {
        goto error;
      }
      if (PyList_SetItem(items, i, item))
      {
        goto error;
      }
      Py_DECREF(key);
    }

    // Store the sorted list of tuples in the newObj slot.
    GET_TC(tc)->newObj = items;
    GET_TC(tc)->size = nitems;
  }

  if (GET_TC(tc)->index >= GET_TC(tc)->size)
  {
    PRINTMARK();
    return 0;
  }

  item = PyList_GetItem(GET_TC(tc)->newObj, GET_TC(tc)->index);
  GET_TC(tc)->itemName = PyTuple_GetItem(item, 0);
  GET_TC(tc)->itemValue = PyTuple_GetItem(item, 1);
  GET_TC(tc)->index++;
  return 1;

error:
  Py_XDECREF(item);
  Py_XDECREF(key);
  Py_XDECREF(value);
  Py_XDECREF(items);
  return -1;
}

static void SortedDict_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
  GET_TC(tc)->itemName = NULL;
  GET_TC(tc)->itemValue = NULL;
  Py_DECREF(GET_TC(tc)->dictObj);
  PRINTMARK();
}

static JSOBJ SortedDict_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
  return GET_TC(tc)->itemValue;
}

static char *SortedDict_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
  *outLen = PyBytes_Size(GET_TC(tc)->itemName);
  return PyBytes_AsString(GET_TC(tc)->itemName);
}

static void SetupDictIter(PyObject *dictObj, TypeContext *pc, JSONObjectEncoder *enc)
{
  pc->dictObj = dictObj;
  if (enc->sortKeys)
  {
    pc->iterEnd = SortedDict_iterEnd;
    pc->iterNext = SortedDict_iterNext;
    pc->iterGetValue = SortedDict_iterGetValue;
    pc->iterGetName = SortedDict_iterGetName;
    pc->index = 0;
  }
  else
  {
    pc->iterEnd = Dict_iterEnd;
    pc->iterNext = Dict_iterNext;
    pc->iterGetValue = Dict_iterGetValue;
    pc->iterGetName = Dict_iterGetName;
    pc->iterator = PyObject_GetIter(dictObj);
  }
}

static void Object_beginTypeContext (JSOBJ _obj, JSONTypeContext *tc, JSONObjectEncoder *enc)
{
  PyObject *obj, *objRepr, *exc;
  TypeContext *pc;
  PRINTMARK();
  if (!_obj)
  {
    tc->type = JT_INVALID;
    return;
  }

  obj = (PyObject*) _obj;

  tc->prv = PyObject_Malloc(sizeof(TypeContext));
  pc = (TypeContext *) tc->prv;
  if (!pc)
  {
    tc->type = JT_INVALID;
    PyErr_NoMemory();
    return;
  }
  pc->newObj = NULL;
  pc->dictObj = NULL;
  pc->itemValue = NULL;
  pc->itemName = NULL;
  pc->iterator = NULL;
  pc->attrList = NULL;
  pc->index = 0;
  pc->size = 0;
  pc->longValue = 0;
  pc->rawJSONValue = NULL;

  if (PyIter_Check(obj))
  {
    PRINTMARK();
    goto ISITERABLE;
  }

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
    pc->PyTypeToJSON = PyLongToINT64;
    tc->type = JT_LONG;
    GET_TC(tc)->longValue = PyLong_AsLongLong(obj);

    exc = PyErr_Occurred();
    if (!exc)
    {
        return;
    }

    if (exc && PyErr_ExceptionMatches(PyExc_OverflowError))
    {
      PyErr_Clear();
      pc->PyTypeToJSON = PyLongToUINT64;
      tc->type = JT_ULONG;
      GET_TC(tc)->unsignedLongValue = PyLong_AsUnsignedLongLong(obj);

      exc = PyErr_Occurred();
      if (exc && PyErr_ExceptionMatches(PyExc_OverflowError))
      {
        PRINTMARK();
        goto INVALID;
      }
    }

    return;
  }
  else
  if (PyLong_Check(obj))
  {
    PRINTMARK();
#ifdef _LP64
    pc->PyTypeToJSON = PyIntToINT64; tc->type = JT_LONG;
#else
    pc->PyTypeToJSON = PyIntToINT32; tc->type = JT_INT;
#endif
    return;
  }
  else
  if (UNLIKELY(PyBytes_Check(obj)))
  {
    PRINTMARK();
    if (enc->rejectBytes)
    {
      PyErr_Format (PyExc_TypeError, "reject_bytes is on and '%s' is bytes", PyBytes_AsString(obj));
      goto INVALID;
    }
    else
    {
      pc->PyTypeToJSON = PyStringToUTF8; tc->type = JT_UTF8;
      return;
    }
  }
  else
  if (PyUnicode_Check(obj))
  {
    PRINTMARK();
    pc->PyTypeToJSON = PyUnicodeToUTF8; tc->type = JT_UTF8;
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
    pc->PyTypeToJSON = PyFloatToDOUBLE; tc->type = JT_DOUBLE;
    return;
  }


ISITERABLE:
  if (PyDict_Check(obj))
  {
    PRINTMARK();
    tc->type = JT_OBJECT;
    SetupDictIter(obj, pc, enc);
    Py_INCREF(obj);
    return;
  }
  else
  if (PyList_Check(obj))
  {
    PRINTMARK();
    tc->type = JT_ARRAY;
    pc->iterEnd = List_iterEnd;
    pc->iterNext = List_iterNext;
    pc->iterGetValue = List_iterGetValue;
    pc->iterGetName = List_iterGetName;
    GET_TC(tc)->index =  0;
    GET_TC(tc)->size = PyList_Size( (PyObject *) obj);
    return;
  }
  else
  if (PyTuple_Check(obj))
  {
    PRINTMARK();
    tc->type = JT_ARRAY;
    pc->iterEnd = Tuple_iterEnd;
    pc->iterNext = Tuple_iterNext;
    pc->iterGetValue = Tuple_iterGetValue;
    pc->iterGetName = Tuple_iterGetName;
    GET_TC(tc)->index = 0;
    GET_TC(tc)->size = PyTuple_Size( (PyObject *) obj);
    GET_TC(tc)->itemValue = NULL;

    return;
  }

  if (UNLIKELY(PyObject_HasAttrString(obj, "toDict")))
  {
    PyObject* toDictFunc = PyObject_GetAttrString(obj, "toDict");
    PyObject* tuple = PyTuple_New(0);
    PyObject* toDictResult = PyObject_Call(toDictFunc, tuple, NULL);
    Py_DECREF(tuple);
    Py_DECREF(toDictFunc);

    if (toDictResult == NULL)
    {
      goto INVALID;
    }

    if (!PyDict_Check(toDictResult))
    {
      Py_DECREF(toDictResult);
      tc->type = JT_NULL;
      return;
    }

    PRINTMARK();
    tc->type = JT_OBJECT;
    SetupDictIter(toDictResult, pc, enc);
    return;
  }
  else
  if (UNLIKELY(PyObject_HasAttrString(obj, "__json__")))
  {
    PyObject* toJSONFunc = PyObject_GetAttrString(obj, "__json__");
    PyObject* tuple = PyTuple_New(0);
    PyObject* toJSONResult = PyObject_Call(toJSONFunc, tuple, NULL);
    Py_DECREF(tuple);
    Py_DECREF(toJSONFunc);

    if (toJSONResult == NULL)
    {
      goto INVALID;
    }

    if (PyErr_Occurred())
    {
      Py_DECREF(toJSONResult);
      goto INVALID;
    }

    if (!PyBytes_Check(toJSONResult) && !PyUnicode_Check(toJSONResult))
    {
      Py_DECREF(toJSONResult);
      PyErr_Format (PyExc_TypeError, "expected string");
      goto INVALID;
    }

    PRINTMARK();
    pc->PyTypeToJSON = PyRawJSONToUTF8;
    tc->type = JT_RAW;
    GET_TC(tc)->rawJSONValue = toJSONResult;
    return;
  }

  PRINTMARK();
  PyErr_Clear();

  objRepr = PyObject_Repr(obj);
  PyObject* str = PyUnicode_AsEncodedString(objRepr, "utf-8", "~E~");
  PyErr_Format (PyExc_TypeError, "%s is not JSON serializable", PyBytes_AsString(str));
  Py_XDECREF(str);
  Py_DECREF(objRepr);

INVALID:
  PRINTMARK();
  tc->type = JT_INVALID;
  PyObject_Free(tc->prv);
  tc->prv = NULL;
  return;
}

static void Object_endTypeContext(JSOBJ obj, JSONTypeContext *tc)
{
  Py_XDECREF(GET_TC(tc)->newObj);

  if (tc->type == JT_RAW)
  {
    Py_XDECREF(GET_TC(tc)->rawJSONValue);
  }
  PyObject_Free(tc->prv);
  tc->prv = NULL;
}

static const char *Object_getStringValue(JSOBJ obj, JSONTypeContext *tc, size_t *_outLen)
{
  return GET_TC(tc)->PyTypeToJSON (obj, tc, NULL, _outLen);
}

static JSINT64 Object_getLongValue(JSOBJ obj, JSONTypeContext *tc)
{
  JSINT64 ret;
  GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static JSUINT64 Object_getUnsignedLongValue(JSOBJ obj, JSONTypeContext *tc)
{
  JSUINT64 ret;
  GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static JSINT32 Object_getIntValue(JSOBJ obj, JSONTypeContext *tc)
{
  JSINT32 ret;
  GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static double Object_getDoubleValue(JSOBJ obj, JSONTypeContext *tc)
{
  double ret;
  GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static void Object_releaseObject(JSOBJ _obj)
{
  Py_DECREF( (PyObject *) _obj);
}

static int Object_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
  return GET_TC(tc)->iterNext(obj, tc);
}

static void Object_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
  GET_TC(tc)->iterEnd(obj, tc);
}

static JSOBJ Object_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
  return GET_TC(tc)->iterGetValue(obj, tc);
}

static char *Object_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
  return GET_TC(tc)->iterGetName(obj, tc, outLen);
}

PyObject* objToJSON(PyObject* self, PyObject *args, PyObject *kwargs)
{
  static char *kwlist[] = { "obj", "ensure_ascii", "encode_html_chars", "escape_forward_slashes", "sort_keys", "indent", "allow_nan", "reject_bytes", NULL };

  char buffer[65536];
  char *ret;
  const char *csNan = NULL, *csInf = NULL;
  PyObject *newobj;
  PyObject *oinput = NULL;
  PyObject *oensureAscii = NULL;
  PyObject *oencodeHTMLChars = NULL;
  PyObject *oescapeForwardSlashes = NULL;
  PyObject *osortKeys = NULL;
  int allowNan = -1;
  int orejectBytes = -1;

  JSONObjectEncoder encoder =
  {
    Object_beginTypeContext,
    Object_endTypeContext,
    Object_getStringValue,
    Object_getLongValue,
    Object_getUnsignedLongValue,
    Object_getIntValue,
    Object_getDoubleValue,
    Object_iterNext,
    Object_iterEnd,
    Object_iterGetValue,
    Object_iterGetName,
    Object_releaseObject,
    PyObject_Malloc,
    PyObject_Realloc,
    PyObject_Free,
    -1, //recursionMax
    1, //forceAscii
    0, //encodeHTMLChars
    1, //escapeForwardSlashes
    0, //sortKeys
    0, //indent
    1, //allowNan
    1, //rejectBytes
    NULL, //prv
  };


  PRINTMARK();

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOiii", kwlist, &oinput, &oensureAscii, &oencodeHTMLChars, &oescapeForwardSlashes, &osortKeys, &encoder.indent, &allowNan, &orejectBytes))
  {
    return NULL;
  }

  if (oensureAscii != NULL && !PyObject_IsTrue(oensureAscii))
  {
    encoder.forceASCII = 0;
  }

  if (oencodeHTMLChars != NULL && PyObject_IsTrue(oencodeHTMLChars))
  {
    encoder.encodeHTMLChars = 1;
  }

  if (oescapeForwardSlashes != NULL && !PyObject_IsTrue(oescapeForwardSlashes))
  {
    encoder.escapeForwardSlashes = 0;
  }

  if (osortKeys != NULL && PyObject_IsTrue(osortKeys))
  {
    encoder.sortKeys = 1;
  }

  if (allowNan != -1)
  {
    encoder.allowNan = allowNan;
  }

  if (encoder.allowNan)
  {
    csInf = "Inf";
    csNan = "NaN";
  }

  if (orejectBytes != -1)
  {
    encoder.rejectBytes = orejectBytes;
  }


  dconv_d2s_init(DCONV_D2S_EMIT_TRAILING_DECIMAL_POINT | DCONV_D2S_EMIT_TRAILING_ZERO_AFTER_POINT | DCONV_D2S_EMIT_POSITIVE_EXPONENT_SIGN,
                 csInf, csNan, 'e', DCONV_DECIMAL_IN_SHORTEST_LOW, DCONV_DECIMAL_IN_SHORTEST_HIGH, 0, 0);

  PRINTMARK();
  ret = JSON_EncodeObject (oinput, &encoder, buffer, sizeof (buffer));
  PRINTMARK();

  dconv_d2s_free();

  if (PyErr_Occurred())
  {
    return NULL;
  }

  if (encoder.errorMsg)
  {
    if (ret != buffer)
    {
      encoder.free (ret);
    }

    PyErr_Format (PyExc_OverflowError, "%s", encoder.errorMsg);
    return NULL;
  }

  newobj = PyUnicode_FromString (ret);

  if (ret != buffer)
  {
    encoder.free (ret);
  }

  PRINTMARK();

  return newobj;
}

PyObject* objToJSONFile(PyObject* self, PyObject *args, PyObject *kwargs)
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

  string = objToJSON (self, argtuple, kwargs);

  if (string == NULL)
  {
    Py_XDECREF(write);
    Py_XDECREF(argtuple);
    return NULL;
  }

  Py_XDECREF(argtuple);

  argtuple = PyTuple_Pack (1, string);
  if (argtuple == NULL)
  {
    Py_XDECREF(write);
    return NULL;
  }

  write_result = PyObject_CallObject (write, argtuple);
  if (write_result == NULL)
  {
    Py_XDECREF(write);
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
