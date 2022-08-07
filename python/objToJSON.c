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

// If newObj is set, we should use it rather than JSOBJ
#define GET_OBJ(__jsobj, __ptrtc) (GET_TC(__ptrtc)->newObj ? GET_TC(__ptrtc)->newObj : __jsobj)

// Avoid infinite loop caused by the default function
#define DEFAULT_FN_MAX_DEPTH 3

struct PyDictIterState
{
  PyObject *keys;
  size_t i;
  size_t sz;
};

//#define PRINTMARK() fprintf(stderr, "%s: MARK(%d)\n", __FILE__, __LINE__)
#define PRINTMARK()

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

static void *PyLongToINTSTR(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  PyObject *obj = PyNumber_ToBase(_obj, 10);
  if (!obj)
  {
    return NULL;
  }
  *_outLen = PyUnicode_GET_LENGTH(obj);
  return PyUnicode_1BYTE_DATA(obj);
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

static char *PyUnicodeToUTF8Raw(JSOBJ _obj, size_t *_outLen, PyObject **pBytesObj)
{
  /*
  Converts the PyUnicode object to char* whose size is stored in _outLen.
  This conversion may require the creation of an intermediate PyBytes object.
  In that case, the returned char* is in fact the internal buffer of that PyBytes object,
  and when the char* buffer is no longer needed, the bytesObj must be DECREF'd.
  */
  PyObject *obj = (PyObject *) _obj;

#ifndef Py_LIMITED_API
  if (PyUnicode_IS_COMPACT_ASCII(obj))
  {
    Py_ssize_t len;
    char *data = PyUnicode_AsUTF8AndSize(obj, &len);
    *_outLen = len;
    return data;
  }
#endif

  PyObject *bytesObj = *pBytesObj = PyUnicode_AsEncodedString (obj, NULL, "surrogatepass");
  if (!bytesObj)
  {
    return NULL;
  }

  *_outLen = PyBytes_Size(bytesObj);
  return PyBytes_AsString(bytesObj);
}

static void *PyUnicodeToUTF8(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
  return PyUnicodeToUTF8Raw(_obj, _outLen, &(GET_TC(tc)->newObj));
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
    GET_TC(tc)->itemName = PyUnicode_AsEncodedString (GET_TC(tc)->itemName, NULL, "surrogatepass");
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

    itemNameTmp = GET_TC(tc)->itemName;
    GET_TC(tc)->itemName = PyObject_Str(GET_TC(tc)->itemName);
    Py_DECREF(itemNameTmp);
    if (PyErr_Occurred())
    {
      PRINTMARK();
      return -1;
    }
    itemNameTmp = GET_TC(tc)->itemName;
    GET_TC(tc)->itemName = PyUnicode_AsEncodedString (GET_TC(tc)->itemName, NULL, "surrogatepass");
    Py_DECREF(itemNameTmp);
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
  // standard library's _json.c sort_keys handler.
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
        key = PyUnicode_AsEncodedString(key, NULL, "surrogatepass");
      }
      else if (!PyBytes_Check(key))
      {
        key = PyObject_Str(key);
        if (PyErr_Occurred())
        {
          goto error;
        }
        keyTmp = key;
        key = PyUnicode_AsEncodedString(key, NULL, "surrogatepass");
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
  PyObject *obj, *objRepr, *exc, *defaultFn, *newObj;
  int level = 0;
  TypeContext *pc;
  PRINTMARK();
  if (!_obj)
  {
    tc->type = JT_INVALID;
    return;
  }

  obj = (PyObject*) _obj;
  defaultFn = (PyObject*) enc->prv;

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

BEGIN:
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
    }

    if (exc && PyErr_ExceptionMatches(PyExc_OverflowError))
    {
      PyErr_Clear();
      pc->PyTypeToJSON = PyLongToINTSTR;
      tc->type = JT_RAW;
      // Overwritten by PyLong_* due to the union, which would lead to a DECREF in endTypeContext.
      GET_TC(tc)->rawJSONValue = NULL;
      return;
    }

    if (exc)
    {
      PRINTMARK();
      goto INVALID;
    }

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

DEFAULT:
  if (defaultFn)
  {
    // Break infinite loop
    if (level >= DEFAULT_FN_MAX_DEPTH)
    {
      PRINTMARK();
      PyErr_Format(PyExc_TypeError, "maximum recursion depth exceeded");
      goto INVALID;
    }

    newObj = PyObject_CallFunctionObjArgs(defaultFn, obj, NULL);
    if (newObj)
    {
      PRINTMARK();
      Py_XDECREF(pc->newObj);
      obj = pc->newObj = newObj;
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
    PyErr_Format (PyExc_TypeError, "%s is not JSON serializable", PyBytes_AsString(str));
  }
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
  obj = GET_OBJ(obj, tc);
  return GET_TC(tc)->PyTypeToJSON (obj, tc, NULL, _outLen);
}

static JSINT64 Object_getLongValue(JSOBJ obj, JSONTypeContext *tc)
{
  JSINT64 ret;
  obj = GET_OBJ(obj, tc);
  GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static JSUINT64 Object_getUnsignedLongValue(JSOBJ obj, JSONTypeContext *tc)
{
  JSUINT64 ret;
  obj = GET_OBJ(obj, tc);
  GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static double Object_getDoubleValue(JSOBJ obj, JSONTypeContext *tc)
{
  double ret;
  obj = GET_OBJ(obj, tc);
  GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
  return ret;
}

static void Object_releaseObject(JSOBJ _obj)
{
  Py_DECREF( (PyObject *) _obj);
}

static int Object_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
  obj = GET_OBJ(obj, tc);
  return GET_TC(tc)->iterNext(obj, tc);
}

static void Object_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
  obj = GET_OBJ(obj, tc);
  GET_TC(tc)->iterEnd(obj, tc);
}

static JSOBJ Object_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
  obj = GET_OBJ(obj, tc);
  return GET_TC(tc)->iterGetValue(obj, tc);
}

static char *Object_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
  obj = GET_OBJ(obj, tc);
  return GET_TC(tc)->iterGetName(obj, tc, outLen);
}

PyObject* objToJSON(PyObject* self, PyObject *args, PyObject *kwargs)
{
  static char *kwlist[] = { "obj", "ensure_ascii", "encode_html_chars", "escape_forward_slashes", "sort_keys", "indent", "allow_nan", "reject_bytes", "default", "separators", NULL };

  char buffer[65536];
  char *ret;
  const char *csNan = NULL, *csInf = NULL;
  PyObject *newobj;
  PyObject *oinput = NULL;
  PyObject *oensureAscii = NULL;
  PyObject *oencodeHTMLChars = NULL;
  PyObject *oescapeForwardSlashes = NULL;
  PyObject *osortKeys = NULL;
  PyObject *odefaultFn = NULL;
  PyObject *oseparators = NULL;
  PyObject *oseparatorsItem = NULL;
  PyObject *separatorsItemBytes = NULL;
  PyObject *oseparatorsKey = NULL;
  PyObject *separatorsKeyBytes = NULL;
  int allowNan = -1;
  int orejectBytes = -1;
  size_t retLen;

  JSONObjectEncoder encoder =
  {
    Object_beginTypeContext,
    Object_endTypeContext,
    Object_getStringValue,
    Object_getLongValue,
    Object_getUnsignedLongValue,
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
    0, //itemSeparatorLength
    NULL, //itemSeparatorChars
    0, //keySeparatorLength
    NULL, //keySeparatorChars
    NULL, //prv
  };


  PRINTMARK();

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOiiiOO", kwlist, &oinput, &oensureAscii, &oencodeHTMLChars, &oescapeForwardSlashes, &osortKeys, &encoder.indent, &allowNan, &orejectBytes, &odefaultFn, &oseparators))
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

  if (odefaultFn != NULL && odefaultFn != Py_None)
  {
    // Here use prv to store default function
    encoder.prv = odefaultFn;
  }

  if (encoder.allowNan)
  {
    csInf = "Infinity";
    csNan = "NaN";
  }

  if (orejectBytes != -1)
  {
    encoder.rejectBytes = orejectBytes;
  }

  if (oseparators != NULL && oseparators != Py_None)
  {
    if (!PyTuple_Check(oseparators))
    {
      PyErr_SetString(PyExc_TypeError, "expected tuple or None as separator");
      return NULL;
    }
    if (PyTuple_Size (oseparators) != 2)
    {
      PyErr_SetString(PyExc_ValueError, "expected tuple of size 2 as separator");
      return NULL;
    }
    oseparatorsItem = PyTuple_GetItem(oseparators, 0);
    if (PyErr_Occurred())
    {
      return NULL;
    }
    if (!PyUnicode_Check(oseparatorsItem))
    {
      PyErr_SetString(PyExc_TypeError, "expected str as item separator");
      return NULL;
    }
    oseparatorsKey = PyTuple_GetItem(oseparators, 1);
    if (PyErr_Occurred())
    {
      return NULL;
    }
    if (!PyUnicode_Check(oseparatorsKey))
    {
      PyErr_SetString(PyExc_TypeError, "expected str as key separator");
      return NULL;
    }
    encoder.itemSeparatorChars = PyUnicodeToUTF8Raw(oseparatorsItem, &encoder.itemSeparatorLength, &separatorsItemBytes);
    if (encoder.itemSeparatorChars == NULL)
    {
      PyErr_SetString(PyExc_ValueError, "item separator malformed");
      goto ERROR;
    }
    encoder.keySeparatorChars = PyUnicodeToUTF8Raw(oseparatorsKey, &encoder.keySeparatorLength, &separatorsKeyBytes);
    if (encoder.keySeparatorChars == NULL)
    {
      PyErr_SetString(PyExc_ValueError, "key separator malformed");
      goto ERROR;
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

  if (encoder.errorMsg && !PyErr_Occurred())
  {
    // If there is an error message and we don't already have a Python exception, set one.
    PyErr_Format (PyExc_OverflowError, "%s", encoder.errorMsg);
  }

  if (PyErr_Occurred())
  {
    if (ret != buffer)
    {
      encoder.free (ret);
    }

    return NULL;
  }

  newobj = PyUnicode_DecodeUTF8(ret, retLen, "surrogatepass");

  if (ret != buffer)
  {
    encoder.free (ret);
  }

  PRINTMARK();

  return newobj;

ERROR:
  Py_XDECREF(separatorsItemBytes);
  Py_XDECREF(separatorsKeyBytes);
  return NULL;
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
