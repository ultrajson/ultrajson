/*
Copyright (c) 2011, Jonas Tarnstrom and ESN Social Software AB
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:
   This product includes software developed by ESN Social Software AB (www.esn.me).
4. Neither the name of the ESN Social Software AB nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY ESN SOCIAL SOFTWARE AB ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ESN SOCIAL SOFTWARE AB BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Portions of code from:
MODP_ASCII - Ascii transformations (upper/lower, etc)
http://code.google.com/p/stringencoders/
Copyright (c) 2007  Nick Galbreath -- nickg [at] modp [dot] com. All rights reserved.

*/

/*
Ultra fast JSON encoder
Developed by Jonas Tärnström (jonas@esn.me).

Notes:

:: Float poing valus ::
All floating point values are converted as doubles using an optimized algorithm
which is close but not entirely IEEE complaint.
Known issues with floating point to string conversion:
x) NaN, -Inf or Inf are not supported
x) Exponents are not supported
x) Max 10 decimals are converted
x) Result might differ in general from IEEE complaint conversion

:: Strings ::
Characters are assumed to be 1 octet. This makes ISO-8859-* 
or UTF8 suitable as input data.

The following characters are escaped:
\"
\\
\/
\b
\f
\n
\r
\t

All other characters are accepted making control characters harmful if present 
in the string octet stream.
The JSON '\uXXXX' conversion is not implemented

:: Integers ::
All integers are converted as signed 64-bit. See JSLONG

:: Cyclic references ::
Cyclic referenced objects are not detected. 
Set JSONObjectEncoder.recursionMax to suitable value or make sure input object 
tree doesn't have cyclic references.

How to implement:

1. Add ultrajson.c and ultrajson.h to your project or makefile
2. Fill out JSONObjectEncoder
3. Call JSON_EncodeObject with root object and a suitable buffer size

Encoding in details:

1. encode(obj):
	1. call getType(obj)
	2. if JT_ARRAY:
		call iterBegin
		while iterNext
			call iterGetValue
			call encode(value)
		call iterEnd

	3. if JT_OBJECT:
		call iterBegin
		while iterNext
			call iterGetName
			call iterGetValue
			call encode (value, name)
		call iterEnd

	4. if JT_*:
		call getValue
		writeOutput

	5. if ti->release is 1:
		call releaseValue

*/

#ifndef __ULTRAJSON_H__
#define __ULTRAJSON_H__

#include <stdio.h>


#define JSON_NO_EXTRA_WHITESPACE

#ifndef JSON_DOUBLE_MAX_DECIMALS
#define JSON_DOUBLE_MAX_DECIMALS 5
#endif

#ifndef JSON_MAX_RECURSION_DEPTH
#define JSON_MAX_RECURSION_DEPTH 256
#endif

#ifdef _WIN32
typedef __int64 JSLONG;
typedef unsigned __int64 JSULONG;
#define EXPORTFUNCTION __declspec(dllexport)
#define INLINEFUNCTION __inline
typedef unsigned __int32 uint32_t;
#else
#include <sys/types.h>
typedef int64_t JSLONG;
typedef u_int64_t JSULONG;
#define INLINEFUNCTION inline
typedef u_int32_t uint32_t;
#define EXPORTFUNCTION
#endif

enum JSTYPES
{
	JT_NULL,		// NULL
	JT_TRUE,		//boolean true
	JT_FALSE,		//boolean false
	JT_INTEGER, //(JSLONG (signed 64-bit))
	JT_DOUBLE,	//(double)
	JT_UTF8,		//(char)
	JT_ARRAY,		// Array structure
	JT_OBJECT,	// Key/Value structure 
	JT_INVALID,	// Internal, do not return nor expect
};

typedef void * JSOBJ;
typedef void * JSITER;

typedef struct __JSTYPEINFO
{
	// If release=1, the releaseValue function will be called with the JSTYPEINFO as argument
	int release; 
	// Type of value see JSTYPES
	int type;
	// Private fields to be used by the implementor for state keeping, life cycle etc
	void *prv[32];
} JSTYPEINFO;

/*
Function pointer declarations, suitable for implementing Ultra JSON */
typedef void (*JSPFN_ITERBEGIN)(JSOBJ obj, JSTYPEINFO *ti);
typedef int (*JSPFN_ITERNEXT)(JSOBJ obj, JSTYPEINFO *ti);
typedef void (*JSPFN_ITEREND)(JSOBJ obj, JSTYPEINFO *ti);
typedef JSOBJ (*JSPFN_ITERGETVALUE)(JSOBJ obj, JSTYPEINFO *ti);
typedef char *(*JSPFN_ITERGETNAME)(JSOBJ obj, JSTYPEINFO *ti, size_t *outLen);
typedef	void *(*JSPFN_MALLOC)(size_t size);
typedef void (*JSPFN_FREE)(void *pptr);
typedef void *(*JSPFN_REALLOC)(void *base, size_t size);

typedef struct __JSONObjectEncoder
{
	/*
	Return type of object as JSTYPES enum 
	Implementors should setup necessary pointers or state in ti->prv
	*/
	void (*getType)(JSOBJ obj, JSTYPEINFO *ti);

	/*
	Get value of object of a specific type

	JT_NULL			: getValue is never called for this type
	JT_TRUE			: getValue is never called for this type
	JT_FALSE		: getValue is never called for this type
	JT_ARRAY		: getValue is never called for this type
	JT_OBJECT		: getValue is never called for this type
	JT_INTEGER, : return NULL, outValue points to a "JSLONG" (64-bit signed), _outLen is ignored
	JT_DOUBLE,	: return NULL, outValue points to a "double", _outLen is ignored
	JT_BOOLEAN,	: return NULL, outValue points to an "int", _outLen is ignored 

	JT_UTF8,		: 
	return pointer to the string buffer
	outValue is ignored
	set _outLen to length of returned string buffer (in bytes without trailing '\0') 

	If it's required that returned resources are freed or released, set ti->release to 1 and releaseValue will be called with JSTYPEINFO as argument.
	Use ti->prv fields to store state for this
	*/
	void *(*getValue)(JSOBJ obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen);

	/*
	Begin iteration of an iteratable object (JS_ARRAY or JS_OBJECT) 
	Implementor should setup iteration state in ti->prv 
	*/
	JSPFN_ITERBEGIN iterBegin;

	/*
	Retrieve next object in an iteration. Should return 0 to indicate iteration has reached end or 1 if there are more items.
	Implementor is responsible for keeping state of the iteration. Use ti->prv fields for this
	*/
	JSPFN_ITERNEXT iterNext;

	/*
	Ends the iteration of an iteratable object.
	Any iteration state stored in ti->prv can be freed here
	*/
	JSPFN_ITEREND iterEnd;

	/*
	Returns a reference to the value object of an iterator
	The is responsible for the life-cycle of the returned string. Use iterNext/iterEnd and ti->prv to keep track of current object
	*/
	JSPFN_ITERGETVALUE iterGetValue;
	
	/*
	Return name of iterator. 
	The is responsible for the life-cycle of the returned string. Use iterNext/iterEnd and ti->prv to keep track of current object
	*/
	JSPFN_ITERGETNAME iterGetName;
	
	/*
	Release a value as indicated by setting ti->release = 1 in the previous getValue call.
	The ti->prv array should contain the necessary context to release the value
	*/
	void (*releaseValue)(JSTYPEINFO *ti);

	/* Library functions 
	Set to NULL to use STDLIB malloc,realloc,free */
	JSPFN_MALLOC malloc;
	JSPFN_REALLOC realloc;
	JSPFN_FREE free;

	/*
	Configuration for max recursion, set to 0 to use default (see JSON_MAX_RECURSION_DEPTH)*/
	int recursionMax;

	/*
	If equals 1 at the end of the encoding the recursionMax limit was hit meaning
	either a recursive object scenario was found or the recursion was too deep */
	int recursionError;

} JSONObjectEncoder;


/*
Encode an object structure into JSON.

Arguments:
obj - An anonymous type representing the object
def - Function definitions for querying JSOBJ type
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
EXPORTFUNCTION char *JSON_EncodeObject(JSOBJ obj, JSONObjectEncoder *def, char *buffer, size_t cbBuffer);



typedef struct __JSONObjectDecoder
{
	JSOBJ (*newString)(char *start, char *end);
	void (*objectAddKey)(JSOBJ obj, JSOBJ name, JSOBJ value);
	void (*arrayAddItem)(JSOBJ obj, JSOBJ value);
	JSOBJ (*newTrue)();
	JSOBJ (*newFalse)();
	JSOBJ (*newNull)();
	JSOBJ (*newObject)();
	JSOBJ (*newArray)();
	JSOBJ (*newInteger)(JSLONG value);
	JSOBJ (*newDouble)(double value);
	JSPFN_MALLOC malloc;
	JSPFN_FREE free;
	JSPFN_REALLOC realloc;
} JSONObjectDecoder;

EXPORTFUNCTION JSOBJ JSON_DecodeObject(JSONObjectDecoder *dec, const char *buffer, size_t cbBuffer);

#endif
