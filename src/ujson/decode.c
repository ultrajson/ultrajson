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
https://github.com/client9/stringencoders
Copyright (c) 2007  Nick Galbreath -- nickg [at] modp [dot] com. All rights reserved.

Numeric decoder derived from from TCL library
https://opensource.apple.com/source/tcl/tcl-14/tcl/license.terms
* Copyright (c) 1988-1993 The Regents of the University of California.
* Copyright (c) 1994 Sun Microsystems, Inc.
*/

#include <stdbool.h>
#include <Python.h>
#include "ultrajson.h"
#include "ujson.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

// Import JSONDecodeError from ujson.c
extern PyObject* JSONDecodeError;

struct DecoderState
{
  char *start;
  char *end;
  uint32_t *escStart;
  uint32_t *escEnd;
  bool escHeap;
  int lastType;
  uint32_t objDepth;
  char *errorStr;
  char *errorOffset;
  void *s2d;
};

static PyObject *FASTCALL_MSVC decode_any( struct DecoderState *ds) FASTCALL_ATTR;

static PyObject *Object_newString(uint32_t *start, uint32_t *end);
static void Object_objectAddKey(PyObject *obj, PyObject *name, PyObject *value);
static void Object_arrayAddItem(PyObject *obj, PyObject *value);
static PyObject *Object_newIntegerFromString(char *value, size_t length);

static PyObject *SetError( struct DecoderState *ds, int offset, const char *message)
{
  ds->errorOffset = ds->start + offset;
  ds->errorStr = (char *) message;
  return NULL;
}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decodeDouble(struct DecoderState *ds)
{
  int processed_characters_count;
  /* Prevent int overflow if ds->end - ds->start is too large. See check_decode_decimal_no_int_overflow()
  inside tests/test_ujson.py for an example where this check is necessary. */
  int len = ((size_t) (ds->end - ds->start) < (size_t) INT_MAX) ? (int) (ds->end - ds->start) : INT_MAX;
  double value = dconv_s2d(ds->s2d, ds->start, len, &processed_characters_count);
  ds->lastType = JT_DOUBLE;
  ds->start += processed_characters_count;
  return PyFloat_FromDouble(value);
}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_numeric (struct DecoderState *ds)
{
  int intNeg = 1;
  bool hasError = false;
  uint64_t intValue;
  uint64_t addIntValue;
  int chr;
  char *offset = ds->start;

  uint64_t maxIntValue = ULLONG_MAX;
  uint64_t overflowLimit = maxIntValue / 10LLU;

  if (*(offset) == 'I')
  {
    goto DECODE_INF;
  }
  else if (*(offset) == 'N')
  {
    goto DECODE_NAN;
  }
  else if (*(offset) == '-')
  {
    offset++;
    intNeg = -1;
    if (*(offset) == 'I')
    {
      goto DECODE_INF;
    }
    maxIntValue = -(uint64_t) LLONG_MIN;
    overflowLimit = maxIntValue / 10LL;
  }

  // Scan integer part
  intValue = 0;

  while (true)
  {
    chr = (int) (unsigned char) *(offset);

    switch (chr)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        // check whether multiplication would be out of bounds
        if (intValue > overflowLimit)
        {
          hasError = true;
        }
        intValue *= 10ULL;
        addIntValue = (uint64_t) (chr - 48);

        // check whether addition would be out of bounds
        if (maxIntValue - intValue < addIntValue)
        {
          hasError = true;
        }

        intValue += addIntValue;
        offset ++;
        break;
      }
      case '.':
      {
        offset ++;
        return decodeDouble(ds);
      }
      case 'e':
      case 'E':
      {
        offset ++;
        return decodeDouble(ds);
      }

      default:
      {
        if (hasError)
        {
          char *strStart = ds->start;
          ds->lastType = JT_INT;
          ds->start = offset;
          return Object_newIntegerFromString(strStart, offset - strStart);
        }
        goto BREAK_INT_LOOP;
        break;
      }
    }
  }

BREAK_INT_LOOP:

  ds->lastType = JT_INT;
  ds->start = offset;

  if (intNeg == 1 && (intValue & 0x8000000000000000ULL) != 0)
  {
    return PyLong_FromUnsignedLongLong(intValue);
  }
  else if ((intValue >> 31))
  {
    return PyLong_FromLongLong((int64_t) (intValue * (int64_t) intNeg));
  }
  else
  {
    return PyLong_FromLong((long) (intValue * intNeg));
  }

DECODE_NAN:
    offset++;
    if (*(offset++) != 'a') goto SET_NAN_ERROR;
    if (*(offset++) != 'N') goto SET_NAN_ERROR;

    ds->lastType = JT_NAN;
    ds->start = offset;
    return PyFloat_FromDouble(Py_NAN);

SET_NAN_ERROR:
    return SetError(ds, -1, "Unexpected character found when decoding 'NaN'");

DECODE_INF:
    offset++;
    if (*(offset++) != 'n') goto SET_INF_ERROR;
    if (*(offset++) != 'f') goto SET_INF_ERROR;
    if (*(offset++) != 'i') goto SET_INF_ERROR;
    if (*(offset++) != 'n') goto SET_INF_ERROR;
    if (*(offset++) != 'i') goto SET_INF_ERROR;
    if (*(offset++) != 't') goto SET_INF_ERROR;
    if (*(offset++) != 'y') goto SET_INF_ERROR;

    ds->start = offset;

    if (intNeg == 1) {
      ds->lastType = JT_POS_INF;
      return PyFloat_FromDouble(Py_HUGE_VAL);
    } else {
      ds->lastType = JT_NEG_INF;
      return PyFloat_FromDouble(-Py_HUGE_VAL);
    }

SET_INF_ERROR:
    if (intNeg == 1) {
      const char *msg = "Unexpected character found when decoding 'Infinity'";
      return SetError(ds, -1, msg);
    } else {
      const char *msg = "Unexpected character found when decoding '-Infinity'";
      return SetError(ds, -1, msg);
    }

}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_true ( struct DecoderState *ds)
{
  char *offset = ds->start;
  offset ++;

  if (*(offset++) != 'r')
    goto SETERROR;
  if (*(offset++) != 'u')
    goto SETERROR;
  if (*(offset++) != 'e')
    goto SETERROR;

  ds->lastType = JT_TRUE;
  ds->start = offset;
  Py_RETURN_TRUE;

SETERROR:
  return SetError(ds, -1, "Unexpected character found when decoding 'true'");
}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_false ( struct DecoderState *ds)
{
  char *offset = ds->start;
  offset ++;

  if (*(offset++) != 'a')
    goto SETERROR;
  if (*(offset++) != 'l')
    goto SETERROR;
  if (*(offset++) != 's')
    goto SETERROR;
  if (*(offset++) != 'e')
    goto SETERROR;

  ds->lastType = JT_FALSE;
  ds->start = offset;
  Py_RETURN_FALSE;

SETERROR:
  return SetError(ds, -1, "Unexpected character found when decoding 'false'");
}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_null ( struct DecoderState *ds)
{
  char *offset = ds->start;
  offset ++;

  if (*(offset++) != 'u')
    goto SETERROR;
  if (*(offset++) != 'l')
    goto SETERROR;
  if (*(offset++) != 'l')
    goto SETERROR;

  ds->lastType = JT_NULL;
  ds->start = offset;
  Py_RETURN_NONE;

SETERROR:
  return SetError(ds, -1, "Unexpected character found when decoding 'null'");
}

static FASTCALL_ATTR void FASTCALL_MSVC SkipWhitespace(struct DecoderState *ds)
{
  char *offset = ds->start;

  for (;;)
  {
    switch (*offset)
    {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        offset ++;
        break;

      default:
        ds->start = offset;
        return;
    }
  }
}

enum DECODESTRINGSTATE
{
  DS_ISNULL = 0x32,
  DS_ISQUOTE,
  DS_ISESCAPE,
  DS_UTFLENERROR,
  DS_CONT,
  DS_EXCEEDSMAX,
};

static const uint8_t g_decoderLookup[256] =
{
  /* 0x00 */ DS_ISNULL, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 0x10 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 0x20 */ 1, 1, DS_ISQUOTE, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 0x30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 0x40 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 0x50 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, DS_ISESCAPE, 1, 1, 1,
  /* 0x60 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 0x70 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 0x80 */ DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT,
  /* 0x90 */ DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT,
  /* 0xa0 */ DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT,
  /* 0xb0 */ DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT, DS_CONT,
  /* 0xc0 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  /* 0xd0 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  /* 0xe0 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  /* 0xf0 */ 4, 4, 4, 4, 4, DS_EXCEEDSMAX, DS_EXCEEDSMAX, DS_EXCEEDSMAX, DS_UTFLENERROR, DS_UTFLENERROR, DS_UTFLENERROR, DS_UTFLENERROR, DS_UTFLENERROR, DS_UTFLENERROR, DS_UTFLENERROR, DS_UTFLENERROR,
};

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_string ( struct DecoderState *ds)
{
  int index;
  uint32_t *escOffset;
  size_t escLen = (ds->escEnd - ds->escStart);
  uint8_t *inputOffset;
  uint16_t ch = 0;
  uint8_t *lastHighSurrogate = NULL;
  uint8_t oct;
  uint32_t ucs;
  ds->lastType = JT_INVALID;
  ds->start ++;

  if ( (size_t) (ds->end - ds->start) > escLen)
  {
    size_t newSize = (ds->end - ds->start);

    uint32_t *oldStart = ds->escStart;
    if (newSize > (SIZE_MAX / sizeof(uint32_t)))
    {
      return SetError(ds, -1, "Could not reserve memory block");
    }
    ds->escStart = (uint32_t *) PyObject_Malloc(newSize * sizeof(uint32_t));
    if (!ds->escStart)
    {
      return SetError(ds, -1, "Could not reserve memory block");
    }
    ds->escHeap = true;
    memcpy(ds->escStart, oldStart, escLen * sizeof(uint32_t));

    ds->escEnd = ds->escStart + newSize;
  }

  escOffset = ds->escStart;
  inputOffset = (uint8_t *) ds->start;

  for (;;)
  {
    switch (g_decoderLookup[(uint8_t)(*inputOffset)])
    {
      case DS_ISNULL:
      {
        return SetError(ds, -1, "Unmatched '\"' when decoding 'string'");
      }
      case DS_ISQUOTE:
      {
        ds->lastType = JT_UTF8;
        inputOffset ++;
        ds->start += ( (char *) inputOffset - (ds->start));
        return Object_newString(ds->escStart, escOffset);
      }
      case DS_UTFLENERROR:
      {
        return SetError (ds, -1, "Invalid UTF-8 sequence length when decoding 'string'");
      }
      case DS_CONT:
      {
        return SetError (ds, -1, "Found UTF-8 continuation byte without corresponding start byte when decoding 'string'");
      }
      case DS_EXCEEDSMAX:
      {
        return SetError(ds, -1, "Code point > U+10FFFF encountered whilst decoding 'string'");
      }
      case DS_ISESCAPE:
      {
        inputOffset ++;
        switch (*inputOffset)
        {
          case '\\': *(escOffset++) = '\\'; inputOffset++; continue;
          case '\"': *(escOffset++) = '\"'; inputOffset++; continue;
          case '/':  *(escOffset++) = '/';  inputOffset++; continue;
          case 'b':  *(escOffset++) = '\b'; inputOffset++; continue;
          case 'f':  *(escOffset++) = '\f'; inputOffset++; continue;
          case 'n':  *(escOffset++) = '\n'; inputOffset++; continue;
          case 'r':  *(escOffset++) = '\r'; inputOffset++; continue;
          case 't':  *(escOffset++) = '\t'; inputOffset++; continue;

          case 'u':
          {
            int index;
            inputOffset ++;

            for (index = 0; index < 4; index ++)
            {
              switch (*inputOffset)
              {
                case '\0': return SetError (ds, -1, "Unterminated unicode escape sequence when decoding 'string'");
                default: return SetError (ds, -1, "Unexpected character in unicode escape sequence when decoding 'string'");

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                  ch = (ch << 4) + (uint16_t) (*inputOffset - '0');
                  break;

                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                  ch = (ch << 4) + 10 + (uint16_t) (*inputOffset - 'a');
                  break;

                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                  ch = (ch << 4) + 10 + (uint16_t) (*inputOffset - 'A');
                  break;
              }

              inputOffset ++;
            }

            if ((ch & 0xfc00) == 0xdc00 && lastHighSurrogate == inputOffset - 6 * sizeof(*inputOffset))
            {
              // Low surrogate immediately following a high surrogate
              // Overwrite existing high surrogate with combined character
              *(escOffset-1) = (((*(escOffset-1) - 0xd800) <<10) | (ch - 0xdc00)) + 0x10000;
            }
            else
            {
              *(escOffset++) = (uint32_t) ch;
            }
            if ((ch & 0xfc00) == 0xd800)
            {
              lastHighSurrogate = inputOffset;
            }
            break;
          }

          case '\0': return SetError(ds, -1, "Unterminated escape sequence when decoding 'string'");
          default: return SetError(ds, -1, "Unrecognized escape sequence when decoding 'string'");
        }
        break;
      }

      case 1:
      {
        *(escOffset++) = (uint32_t) (*inputOffset++);
        break;
      }

      case 2:
      {
        ucs = (*inputOffset++) & 0x1f;
        ucs <<= 6;
        if (((*inputOffset) & 0xc0) != 0x80)
        {
          return SetError(ds, -1, "Invalid UTF-8 continuation byte when decoding 'string'");
        }
        ucs |= (*inputOffset++) & 0x3f;
        if (ucs < 0x80) return SetError (ds, -1, "Overlong 2-byte UTF-8 sequence detected when decoding 'string'");
        *(escOffset++) = (uint32_t) ucs;
        break;
      }

      case 3:
      {
        uint32_t ucs = 0;
        ucs |= (*inputOffset++) & 0x0f;

        for (index = 0; index < 2; index ++)
        {
          ucs <<= 6;
          oct = (*inputOffset++);

          if ((oct & 0xc0) != 0x80)
          {
            return SetError(ds, -1, "Invalid UTF-8 continuation byte when decoding 'string'");
          }

          ucs |= oct & 0x3f;
        }

        if (ucs < 0x800) return SetError (ds, -1, "Overlong 3-byte UTF-8 sequence detected when encoding string");
        *(escOffset++) = (uint32_t) ucs;
        break;
      }

      case 4:
      {
        uint32_t ucs = 0;
        ucs |= (*inputOffset++) & 0x07;

        for (index = 0; index < 3; index ++)
        {
          ucs <<= 6;
          oct = (*inputOffset++);

          if ((oct & 0xc0) != 0x80)
          {
            return SetError(ds, -1, "Invalid UTF-8 continuation byte when decoding 'string'");
          }

          ucs |= oct & 0x3f;
        }

        if (ucs < 0x10000) return SetError (ds, -1, "Overlong 4-byte UTF-8 sequence detected when decoding 'string'");
        if (ucs > 0x10FFFF) return SetError(ds, -1, "Code point > U+10FFFF encountered whilst decoding 'string'");

        *(escOffset++) = (uint32_t) ucs;
        break;
      }
    }
  }
}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_array(struct DecoderState *ds)
{
  PyObject *itemValue;
  PyObject *newObj;
  int len;
  ds->objDepth++;
  if (ds->objDepth > JSON_MAX_OBJECT_DEPTH) {
    return SetError(ds, -1, "Reached object decoding depth limit");
  }

  newObj = PyList_New(0);
  len = 0;

  ds->lastType = JT_INVALID;
  ds->start ++;

  for (;;)
  {
    SkipWhitespace(ds);

    if ((*ds->start) == ']')
    {
      ds->objDepth--;
      if (len == 0)
      {
        ds->start ++;
        return newObj;
      }

      Py_DECREF(newObj);
      return SetError(ds, -1, "Unexpected character found when decoding array value (1)");
    }

    itemValue = decode_any(ds);

    if (itemValue == NULL)
    {
      Py_DECREF(newObj);
      return NULL;
    }

    Object_arrayAddItem(newObj, itemValue);

    SkipWhitespace(ds);

    switch (*(ds->start++))
    {
    case ']':
    {
      ds->objDepth--;
      return newObj;
    }
    case ',':
      break;

    default:
      Py_DECREF(newObj);
      return SetError(ds, -1, "Unexpected character found when decoding array value (2)");
    }

    len++;
  }
}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_object( struct DecoderState *ds)
{
  PyObject *itemName;
  PyObject *itemValue;
  PyObject *newObj;
  int len;

  ds->objDepth++;
  if (ds->objDepth > JSON_MAX_OBJECT_DEPTH)
  {
    return SetError(ds, -1, "Reached object decoding depth limit");
  }

  newObj = PyDict_New();
  len = 0;

  ds->start ++;

  for (;;)
  {
    SkipWhitespace(ds);

    if ((*ds->start) == '}')
    {
      ds->objDepth--;
      if (len == 0)
      {
        ds->start ++;
        return newObj;
      }

      Py_DECREF(newObj);
      return SetError(ds, -1, "Unexpected character in found when decoding object value");
    }

    ds->lastType = JT_INVALID;
    itemName = decode_any(ds);

    if (itemName == NULL)
    {
      Py_DECREF(newObj);
      return NULL;
    }

    if (ds->lastType != JT_UTF8)
    {
      Py_DECREF(newObj);
      Py_DECREF(itemName);
      return SetError(ds, -1, "Key name of object must be 'string' when decoding 'object'");
    }

    SkipWhitespace(ds);

    if (*(ds->start++) != ':')
    {
      Py_DECREF(newObj);
      Py_DECREF(itemName);
      return SetError(ds, -1, "No ':' found when decoding object value");
    }

    SkipWhitespace(ds);

    itemValue = decode_any(ds);

    if (itemValue == NULL)
    {
      Py_DECREF(newObj);
      Py_DECREF(itemName);
      return NULL;
    }

    Object_objectAddKey(newObj, itemName, itemValue);

    SkipWhitespace(ds);

    switch (*(ds->start++))
    {
      case '}':
      {
        ds->objDepth--;
        return newObj;
      }
      case ',':
        break;

      default:
        Py_DECREF(newObj);
        return SetError(ds, -1, "Unexpected character in found when decoding object value");
    }

    len++;
  }
}

static FASTCALL_ATTR PyObject *FASTCALL_MSVC decode_any(struct DecoderState *ds)
{
  for (;;)
  {
    switch (*ds->start)
    {
      case '\"':
        return decode_string (ds);
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'I':
      case 'N':
      case '-':
        return decode_numeric (ds);

      case '[': return decode_array (ds);
      case '{': return decode_object (ds);
      case 't': return decode_true (ds);
      case 'f': return decode_false (ds);
      case 'n': return decode_null (ds);

      case ' ':
      case '\t':
      case '\r':
      case '\n':
        // White space
        ds->start ++;
        break;

      default:
        return SetError(ds, -1, "Expected object or value");
    }
  }
}

static void Object_objectAddKey(PyObject *obj, PyObject *name, PyObject *value)
{
  int result = PyDict_SetItem(obj, name, value);
  if (result == -1) {
    // Clear the Python error state to prevent SystemError
    PyErr_Clear();
    // Set our own error
    PyErr_SetString(JSONDecodeError, "Invalid JSON: object keys must be strings");
  }
  Py_DECREF(name);
  Py_DECREF(value);
  return;
}

static void Object_arrayAddItem(PyObject *obj, PyObject *value)
{
  PyList_Append(obj, value);
  Py_DECREF(value);
  return;
}

/*
Check that Py_UCS4 is the same as uint32_t, else Object_newString will fail.
Based on Linux's check in vbox_vmmdev_types.h.
This should be replaced with
  _Static_assert(sizeof(Py_UCS4) == sizeof(uint32_t));
when C11 is made mandatory (CPython 3.11+, PyPy ?).
*/
typedef char assert_py_ucs4_is_jsuint32[1 - 2*!(sizeof(Py_UCS4) == sizeof(uint32_t))];

static PyObject *Object_newString(uint32_t *start, uint32_t *end)
{
  return PyUnicode_FromKindAndData (PyUnicode_4BYTE_KIND, (Py_UCS4 *) start, (end - start));
}

static PyObject *Object_newIntegerFromString(char *value, size_t length)
{
  // PyLong_FromString requires a NUL-terminated string in CPython, contrary to the documentation: https://github.com/python/cpython/issues/59200
  char *buf = PyObject_Malloc(length + 1);
  memcpy(buf, value, length);
  buf[length] = '\0';
  PyObject *ret = PyLong_FromString(buf, NULL, 10);
  PyObject_Free(buf);
  return ret;
}

static char *g_kwlist[] = {"obj", NULL};

PyObject* ujson_loads(PyObject* self, PyObject *args, PyObject *kwargs)
{
  PyObject *ret;
  PyObject *sarg = NULL;
  PyObject *arg;
  void *s2d = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", g_kwlist, &arg))
  {
      return NULL;
  }

  Py_buffer buffer;
  size_t sarg_length;
  char * raw;
  bool is_bytes_like = !PyObject_GetBuffer(arg, &buffer, PyBUF_C_CONTIGUOUS);
  if (is_bytes_like)
  {
    if (!PyBytes_Check(arg) && !PyByteArray_Check(arg)) {
      PyBuffer_Release(&buffer);
      PyErr_Format(PyExc_TypeError, "Arbitrary bytes-like objects are no longer supported. Use either string, bytes, or bytearray.");
      return NULL;
    }
    raw = buffer.buf;
    sarg_length = buffer.len;
  }
  else
  {
    PyErr_Clear();
    if (PyUnicode_Check(arg))
    {
      sarg = PyUnicode_AsEncodedString(arg, NULL, "surrogatepass");
      if (sarg == NULL)
      {
        // Exception raisable above us by codec only through out of memory error
        return NULL;
      }
      sarg_length = PyBytes_Size(sarg);
      raw = PyBytes_AsString(sarg);
    }
    else
    {
      PyErr_Format(PyExc_TypeError, "Expected string, bytes, or bytearray");
      return NULL;
    }
  }

  dconv_s2d_init(&s2d, DCONV_S2D_ALLOW_TRAILING_JUNK, 0.0, 0.0, "Infinity", "NaN");

  // FIXME: Base the size of escBuffer of that of cbBuffer so that the unicode escaping doesn't run into the wall each time
  struct DecoderState ds;
  uint32_t escBuffer[(JSON_MAX_STACK_BUFFER_SIZE / sizeof(uint32_t))];
  ds.start = (char *) raw;
  ds.end = ds.start + sarg_length;
  ds.escStart = escBuffer;
  ds.escEnd = ds.escStart + (JSON_MAX_STACK_BUFFER_SIZE / sizeof(uint32_t));
  ds.escHeap = false;
  ds.errorStr = NULL;
  ds.errorOffset = NULL;
  ds.objDepth = 0;
  ds.s2d = s2d;

  ret = decode_any (&ds);

  if (ds.escHeap)
  {
    PyObject_Free(ds.escStart);
  }

  if (!(ds.errorStr))
  {
    if ((ds.end - ds.start) > 0)
    {
      SkipWhitespace(&ds);
    }

    if (ds.start != ds.end && ret)
    {
      Py_DECREF(ret);
      ret = SetError(&ds, -1, "Trailing data");
    }
  }

  dconv_s2d_free(&s2d);

  if (!is_bytes_like)
  {
    Py_DECREF(sarg);
  }
  else
  {
    PyBuffer_Release(&buffer);
  }

  // Check if a Python error occurred during decoding
  if (PyErr_Occurred())
  {
    if (ret)
    {
        Py_DECREF(ret);
    }
    return NULL;
  }

  if (ds.errorStr)
  {
    /*
    FIXME: It's possible to give a much nicer error message here with actual failing element in input etc*/

    PyErr_Format (JSONDecodeError, "%s", ds.errorStr);

    if (ret)
    {
        Py_DECREF(ret);
    }

    return NULL;
  }

  return ret;
}

PyObject* ujson_load(PyObject* self, PyObject *args, PyObject *kwargs)
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

  result = ujson_loads(self, argtuple, kwargs);

  Py_XDECREF(argtuple);
  Py_XDECREF(string);

  if (result == NULL) {
    return NULL;
  }

  return result;
}
