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

/*
Ultra fast JSON encoder and decoder
Developed by Jonas Tarnstrom (jonas@esn.me).

Encoder notes:
------------------

:: Cyclic references ::
Cyclic referenced objects are not detected.
Set JSONObjectEncoder.recursionMax to suitable value or make sure input object
tree doesn't have cyclic references.

*/

#ifndef __ULTRAJSON_H__
#define __ULTRAJSON_H__

#include <stdio.h>
#include <wchar.h>

// Don't output any extra whitespaces when encoding
#define JSON_NO_EXTRA_WHITESPACE

// Max decimals to encode double floating point numbers with
#ifndef JSON_DOUBLE_MAX_DECIMALS
#define JSON_DOUBLE_MAX_DECIMALS 15
#endif

// Max recursion depth, default for encoder
#ifndef JSON_MAX_RECURSION_DEPTH
#define JSON_MAX_RECURSION_DEPTH 1024
#endif

// Max recursion depth, default for decoder
#ifndef JSON_MAX_OBJECT_DEPTH
#define JSON_MAX_OBJECT_DEPTH 1024
#endif

/*
Dictates and limits how much stack space for buffers UltraJSON will use before resorting to provided heap functions */
#ifndef JSON_MAX_STACK_BUFFER_SIZE
#define JSON_MAX_STACK_BUFFER_SIZE 131072
#endif

#ifdef _WIN32

typedef __int64 JSINT64;
typedef unsigned __int64 JSUINT64;

typedef __int32 JSINT32;
typedef unsigned __int32 JSUINT32;
typedef unsigned __int8 JSUINT8;
typedef unsigned __int16 JSUTF16;
typedef unsigned __int32 JSUTF32;
typedef __int64 JSLONG;

#define EXPORTFUNCTION __declspec(dllexport)

#define FASTCALL_MSVC __fastcall
#define FASTCALL_ATTR
#define INLINE_PREFIX __inline

#else

#include <stdint.h>
typedef int64_t JSINT64;
typedef uint64_t JSUINT64;

typedef int32_t JSINT32;
typedef uint32_t JSUINT32;

#define FASTCALL_MSVC

#if !defined __x86_64__
#define FASTCALL_ATTR __attribute__((fastcall))
#else
#define FASTCALL_ATTR
#endif

#define INLINE_PREFIX inline

typedef uint8_t JSUINT8;
typedef uint16_t JSUTF16;
typedef uint32_t JSUTF32;

typedef int64_t JSLONG;

#define EXPORTFUNCTION
#endif

#if !(defined(__LITTLE_ENDIAN__) || defined(__BIG_ENDIAN__))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#else

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif

#endif

#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#error "Endianess not supported"
#endif

enum JSTYPES
{
  JT_NULL,      // NULL
  JT_TRUE,      // boolean true
  JT_FALSE,     // boolean false
  JT_INT,       // (JSINT32 (signed 32-bit))
  JT_LONG,      // (JSINT64 (signed 64-bit))
  JT_ULONG,     // (JSUINT64 (unsigned 64-bit))
  JT_DOUBLE,    // (double)
  JT_UTF8,      // (char 8-bit)
  JT_RAW,       // (raw char 8-bit)
  JT_ARRAY,     // Array structure
  JT_OBJECT,    // Key/Value structure
  JT_INVALID,   // Internal, do not return nor expect
};

typedef void * JSOBJ;
typedef void * JSITER;

typedef struct __JSONTypeContext
{
  int type;
  void *prv;
  void *encoder_prv;
} JSONTypeContext;

/*
Function pointer declarations, suitable for implementing UltraJSON */
typedef int (*JSPFN_ITERNEXT)(JSOBJ obj, JSONTypeContext *tc);
typedef void (*JSPFN_ITEREND)(JSOBJ obj, JSONTypeContext *tc);
typedef JSOBJ (*JSPFN_ITERGETVALUE)(JSOBJ obj, JSONTypeContext *tc);
typedef char *(*JSPFN_ITERGETNAME)(JSOBJ obj, JSONTypeContext *tc, size_t *outLen);
typedef void *(*JSPFN_MALLOC)(size_t size);
typedef void (*JSPFN_FREE)(void *pptr);
typedef void *(*JSPFN_REALLOC)(void *base, size_t size);


struct __JSONObjectEncoder;

typedef struct __JSONObjectEncoder
{
  /*
  Configuration for max recursion, set to 0 to use default (see JSON_MAX_RECURSION_DEPTH)*/
  int recursionMax;

  /*
  Configuration for max decimals of double floating point numbers to encode (0-9) */
  int doublePrecision;

  /*
  If true output will be ASCII with all characters above 127 encoded as \uXXXX. If false output will be UTF-8 or what ever charset strings are brought as */
  int forceASCII;

  /*
  If true, '<', '>', and '&' characters will be encoded as \u003c, \u003e, and \u0026, respectively. If false, no special encoding will be used. */
  int encodeHTMLChars;

  /*
  If true, '/' will be encoded as \/. If false, no escaping. */
  int escapeForwardSlashes;

  /*
  If true, dictionaries are iterated through in sorted key order. */
  int sortKeys;

  /*
  Configuration for spaces of indent */
  int indent;

  /*
  Private pointer to be used by the caller. Passed as encoder_prv in JSONTypeContext */
  void *prv;

  /*
  Set to an error message if error occured */
  const char *errorMsg;
  JSOBJ errorObj;

  /* Buffer stuff */
  char *start;
  char *offset;
  char *end;
  int heap;
  int level;

} JSONObjectEncoder;


/*
Encode an object structure into JSON.

Arguments:
obj - An anonymous type representing the object
enc - Function definitions for querying JSOBJ type
buffer - Preallocated buffer to store result in. If NULL function allocates own buffer
cbBuffer - Length of buffer (ignored if buffer is NULL)

Returns:
Encoded JSON object as a null terminated char string.

NOTE:
If the supplied buffer wasn't enough to hold the result the function will allocate a new buffer.
Life cycle of the provided buffer must still be handled by caller.

If the return value doesn't equal the specified buffer caller must release the memory using
JSONObjectEncoder.free or free() as specified when calling this function.
*/
EXPORTFUNCTION char *JSON_EncodeObject(JSOBJ obj, JSONObjectEncoder *enc, char *buffer, size_t cbBuffer);



typedef struct __JSONObjectDecoder
{
  char *errorStr;
  char *errorOffset;
  int preciseFloat;
  void *prv;
} JSONObjectDecoder;




EXPORTFUNCTION JSOBJ JSON_DecodeObject(JSONObjectDecoder *dec, const char *buffer, size_t cbBuffer);

JSOBJ ujdecode_newString(void *prv, wchar_t *start, wchar_t *end);
void ujdecode_objectAddKey(void *prv, JSOBJ obj, JSOBJ name, JSOBJ value);
void ujdecode_arrayAddItem(void *prv, JSOBJ obj, JSOBJ value);
JSOBJ ujdecode_newTrue(void *prv);
JSOBJ ujdecode_newFalse(void *prv);
JSOBJ ujdecode_newNull(void *prv);
JSOBJ ujdecode_newObject(void *prv);
JSOBJ ujdecode_newArray(void *prv);
JSOBJ ujdecode_newInt(void *prv, JSINT32 value);
JSOBJ ujdecode_newLong(void *prv, JSINT64 value);
JSOBJ ujdecode_newUnsignedLong(void *prv, JSUINT64 value);
JSOBJ ujdecode_newDouble(void *prv, double value);
void ujdecode_releaseObject(void *prv, JSOBJ obj);
void *ujdecode_malloc(size_t size);
void ujdecode_free(void *pptr);
void *ujdecode_realloc(void *base, size_t size);

void ujencode_beginTypeContext(JSOBJ obj, JSONTypeContext *tc, struct __JSONObjectEncoder *enc);
void ujencode_endTypeContext(JSOBJ obj, JSONTypeContext *tc);
const char *ujencode_getStringValue(JSOBJ obj, JSONTypeContext *tc, size_t *_outLen);
JSINT64 ujencode_getLongValue(JSOBJ obj, JSONTypeContext *tc);
JSUINT64 ujencode_getUnsignedLongValue(JSOBJ obj, JSONTypeContext *tc);
JSINT32 ujencode_getIntValue(JSOBJ obj, JSONTypeContext *tc);
double ujencode_getDoubleValue(JSOBJ obj, JSONTypeContext *tc);
void ujencode_releaseObject(JSOBJ obj);
int ujencode_iterNext(JSOBJ obj, JSONTypeContext *tc);
void ujencode_iterEnd(JSOBJ obj, JSONTypeContext *tc);
JSOBJ ujencode_iterGetValue(JSOBJ obj, JSONTypeContext *tc);
char *ujencode_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen);
void *ujencode_malloc(size_t size);
void ujencode_free(void *pptr);
void *ujencode_realloc(void *base, size_t size);

#endif
