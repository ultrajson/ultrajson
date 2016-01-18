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
#include <datetime.h>

//#define PRINTMARK() fprintf(stderr, "%s: MARK(%d)\n", __FILE__, __LINE__)
#define PRINTMARK()

void Object_objectAddKey(void *prv, JSOBJ obj, JSOBJ name, JSOBJ value)
{
  PyDict_SetItem (obj, name, value);
  Py_DECREF( (PyObject *) name);
  Py_DECREF( (PyObject *) value);
  return;
}

void Object_arrayAddItem(void *prv, JSOBJ obj, JSOBJ value)
{
  PyList_Append(obj, value);
  Py_DECREF( (PyObject *) value);
  return;
}

JSOBJ Object_newString(void *prv, wchar_t *start, wchar_t *end, int decode_datetime)
{
  int i, year, month, day, hour, minutes, seconds, microseconds, isfulldatetime;
  char *p, *array[3], *buff;

  if (decode_datetime && start[4] == '-')/*we know that the format of the date datetime is 2015-12-14 16:59:51:333*/
  {
    buff = malloc(end - start + 1);
    for (i = 0; i < end - start; i++) {
      buff[i] = start[i];
    }
    start[i++] = '\0';
    for(i=0; i<3; i++)
    {
      array[i] = NULL;
    }
    i = 0;
    p = strtok(buff, " ");
    while (p != NULL) /*split the string into 2015-12-14 and 16:59:51:333*/
    {
      array[i++] = p;
      p = strtok(NULL, " ");
    }
    p = strtok(array[0], "-");
    i = 0;  year = 0; month = 0; day = 0;
    isfulldatetime = 0;
    while (p != NULL)/* parse 2015-12-14 */
    {
      switch (i) {

      case 0:
        year = atoi(p);
        break;

      case 1:
        month = atoi(p);
        break;
      case 2:
        day = atoi(p);
        break;

      }

      p = strtok(NULL, "-");
      i++;
    }

    p = strtok(array[1], ":");
    i = 0; hour = 0; minutes = 0; seconds = 0; microseconds = 0;
    while (p != NULL) /* parse 16:59:51:333 */
    {
      switch (i) {

      case 0:
        isfulldatetime = 1;
        hour = atoi(p);
        break;
      case 1:
        minutes = atoi(p);
        isfulldatetime = 1;
        break;
      case 2:
        seconds = atoi(p);
        isfulldatetime = 1;
        break;
      case 3:
        microseconds = atoi(p);
        isfulldatetime = 1;
        break;

      }
      p = strtok(NULL, ":");
      i++;
    }
    free(buff);

    /* the problem is here we convert the datetime.time to datetime.datetime. We have
     * to do it, because we may have a datetime 2015-12-12 0:0:0:0
     */
    if (isfulldatetime)
    {
      return PyDateTime_FromDateAndTime(year, month, day, hour, minutes,seconds, microseconds);
    }
    else
    {
      return PyDateTime_FromDateAndTime(year, month, day, 0, 0,0, 0);
    }
  }
  return PyUnicode_FromWideChar(start, (end - start));
}

JSOBJ Object_newTrue(void *prv)
{
  Py_RETURN_TRUE;
}

JSOBJ Object_newFalse(void *prv)
{
  Py_RETURN_FALSE;
}

JSOBJ Object_newNull(void *prv)
{
  Py_RETURN_NONE;
}

JSOBJ Object_newObject(void *prv)
{
  return PyDict_New();
}

JSOBJ Object_newArray(void *prv)
{
  return PyList_New(0);
}

JSOBJ Object_newInteger(void *prv, JSINT32 value)
{
  return PyInt_FromLong( (long) value);
}

JSOBJ Object_newLong(void *prv, JSINT64 value)
{
  return PyLong_FromLongLong (value);
}

JSOBJ Object_newUnsignedLong(void *prv, JSUINT64 value)
{
  return PyLong_FromUnsignedLongLong (value);
}

JSOBJ Object_newDouble(void *prv, double value)
{
  return PyFloat_FromDouble(value);
}

static void Object_releaseObject(void *prv, JSOBJ obj)
{
  Py_DECREF( ((PyObject *)obj));
}

static char *g_kwlist[] = {"obj", "precise_float", "decode_datetime", NULL};

PyObject* JSONToObj(PyObject* self, PyObject *args, PyObject *kwargs)
{
  PyObject *ret;
  PyObject *sarg;
  PyObject *arg;
  PyObject *opreciseFloat = NULL;
  PyObject *odecodeDatetimeToString = NULL;
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
  PyDateTime_IMPORT;
  decoder.preciseFloat = 0;
  decoder.stringToDatetime = 0;
  decoder.prv = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO", g_kwlist, &arg, &opreciseFloat, &odecodeDatetimeToString))
  {
      return NULL;
  }

  if (opreciseFloat && PyObject_IsTrue(opreciseFloat))
  {
      decoder.preciseFloat = 1;
  }

  if (odecodeDatetimeToString != NULL && PyObject_IsTrue(odecodeDatetimeToString))
  {
    decoder.stringToDatetime = 1;
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
