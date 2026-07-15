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
#include <stddef.h>
#include <stdbool.h>

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
    #define JSON_MAX_STACK_BUFFER_SIZE 1024
#endif

#ifdef _WIN32
    #define FASTCALL_MSVC __fastcall
    #define FASTCALL_ATTR
    #define INLINE_PREFIX __inline
#else
    #define FASTCALL_MSVC
    #if !defined __x86_64__
        #define FASTCALL_ATTR __attribute__((fastcall))
    #else
        #define FASTCALL_ATTR
    #endif
    #define INLINE_PREFIX inline
#endif

#ifdef __GNUC__
    #define LIKELY(x)       __builtin_expect(!!(x), 1)
    #define UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)       (x)
    #define UNLIKELY(x)     (x)
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
    #error "Endianness not supported"
#endif

enum JSTYPES
{
  JT_NULL,      // NULL
  JT_TRUE,      // boolean true
  JT_FALSE,     // boolean false
  JT_INT,       // (int32_t (signed 32-bit))
  JT_LONG,      // (int64_t (signed 64-bit))
  JT_ULONG,     // (uint64_t (unsigned 64-bit))
  JT_DOUBLE,    // (double)
  JT_UTF8,      // (char 8-bit)
  JT_RAW,       // (raw char 8-bit)
  JT_ARRAY,     // Array structure
  JT_OBJECT,    // Key/Value structure
  JT_INVALID,   // Internal, do not return nor expect
  JT_NAN,       // Not A Number
  JT_POS_INF,   // Positive infinity
  JT_NEG_INF,   // Negative infinity
};

struct __JSONObjectEncoder;

typedef struct __JSONObjectEncoder
{
  /*
  Configuration for max recursion, set to 0 to use default (see JSON_MAX_RECURSION_DEPTH)*/
  int recursionMax;

  /*
  If true output will be ASCII with all characters above 127 encoded as \uXXXX. If false output will be UTF-8 or what ever charset strings are brought as */
  bool forceASCII;

  /*
  If true, '<', '>', and '&' characters will be encoded as \u003c, \u003e, and \u0026, respectively. If false, no special encoding will be used. */
  bool encodeHTMLChars;

  /*
  If true, '/' will be encoded as \/. If false, no escaping. */
  bool escapeForwardSlashes;

  /*
  If true, dictionaries are iterated through in sorted key order. */
  bool sortKeys;

  /*
  Configuration for spaces of indent */
  ptrdiff_t indent;

  /*
  If true, NaN will be encoded as a string matching the Python standard library's JSON behavior.
  This is not valid JSON. */
  bool allowNan;

  /*
  If true, bytes are rejected. */
  bool rejectBytes;

  /*
  Configuration for item and key separators, e.g. "," and ":" for a compact representation or ", " and ": " to match the Python standard library's defaults. */
  size_t itemSeparatorLength;
  const char *itemSeparatorChars;
  size_t keySeparatorLength;
  const char *keySeparatorChars;

  /*
  The function passed to ujson.dumps()'s "default" prameter. */
  PyObject *defaultFn;

  /*
  Pointer to the DoubleToStringConverter instance */
  void *d2s;

  /*
  Set to an error message if error occurred */
  const char *errorMsg;
  PyObject *errorObj;

  /* Buffer stuff */
  char *start;
  char *offset;
  char *end;
  bool heap;
  int level;

} JSONObjectEncoder;


/*
Encode an object structure into JSON.

Arguments:
obj - An anonymous type representing the object
enc - Function definitions for querying PyObject type
buffer - Preallocated buffer to store result in. If NULL function allocates own buffer
cbBuffer - Length of buffer (ignored if buffer is NULL)
outLen - Will store the length of the encoded string

Returns:
Encoded JSON object as a char string.

NOTE:
If the supplied buffer wasn't enough to hold the result the function will allocate a new buffer.
Life cycle of the provided buffer must still be handled by caller.

If the return value doesn't equal the specified buffer caller must release the memory using
JSONObjectEncoder.free or free() as specified when calling this function.

If an error occurs during encoding, NULL is returned and no outLen is stored.
*/

#define DCONV_DECIMAL_IN_SHORTEST_LOW -4
#define DCONV_DECIMAL_IN_SHORTEST_HIGH 16

enum dconv_d2s_flags {
  DCONV_D2S_NO_FLAGS = 0,
  DCONV_D2S_EMIT_POSITIVE_EXPONENT_SIGN = 1,
  DCONV_D2S_EMIT_TRAILING_DECIMAL_POINT = 2,
  DCONV_D2S_EMIT_TRAILING_ZERO_AFTER_POINT = 4,
  DCONV_D2S_UNIQUE_ZERO = 8
};

enum dconv_s2d_flags
{
  DCONV_S2D_NO_FLAGS = 0,
  DCONV_S2D_ALLOW_HEX = 1,
  DCONV_S2D_ALLOW_OCTALS = 2,
  DCONV_S2D_ALLOW_TRAILING_JUNK = 4,
  DCONV_S2D_ALLOW_LEADING_SPACES = 8,
  DCONV_S2D_ALLOW_TRAILING_SPACES = 16,
  DCONV_S2D_ALLOW_SPACES_AFTER_SIGN = 32
};

void dconv_d2s_init(void **d2s,
                    int flags,
                    const char* infinity_symbol,
                    const char* nan_symbol,
                    char exponent_character,
                    int decimal_in_shortest_low,
                    int decimal_in_shortest_high,
                    int max_leading_padding_zeroes_in_precision_mode,
                    int max_trailing_padding_zeroes_in_precision_mode);
bool dconv_d2s(void *d2s, double value, char* buf, int buflen, int* strlength);
void dconv_d2s_free(void **d2s);

void dconv_s2d_init(void **s2d, int flags, double empty_string_value,
                    double junk_string_value, const char* infinity_symbol,
                    const char* nan_symbol);
double dconv_s2d(void *s2d, const char* buffer, int length, int* processed_characters_count);
void dconv_s2d_free(void **s2d);

#endif
