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
DISCLAIMED. IN NO EVENT SHALLESN SOCIAL SOFTWARE AB BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ultrajson.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#ifdef _WIN32
#define INLINEFUNCTION __inline
#else
#error "Fix inline definition for non MSVC"
#endif

typedef struct _Buffer
{
	char *start;
	char *offset;
	char *end;
	int heap;
	JSPFN_MALLOC malloc;
	JSPFN_REALLOC realloc;
	JSPFN_FREE free;

} Buffer;

static void Buffer_Realloc (Buffer *buffer);

#define FastMemCopy memcpy

#define Buffer_AppendEscape(__buffer, __pstr, __len)											\
				while ((__buffer)->offset + ((__len * 2) + 2) > (__buffer)->end)	\
				{																																	\
					Buffer_Realloc((__buffer));																			\
				}																																	\
				*((__buffer)->offset++) = '\"';																		\
				Buffer_Escape((__buffer), (char *)__pstr, __len);									\
				*((__buffer)->offset++) = '\"';																		

#define Buffer_AppendEscapeUnchecked(__buffer, __pstr, __len)							\
				*((__buffer)->offset++) = '\"';																		\
				Buffer_Escape((__buffer), (char *)__pstr, __len);									\
				*((__buffer)->offset++) = '\"';																		


#define Buffer_Append(__buffer, __pstr, __len)														\
				while ((__buffer)->offset + __len > (__buffer)->end)							\
				{																																	\
					Buffer_Realloc((__buffer));																			\
				}																																	\
				FastMemCopy ((__buffer)->offset, __pstr, __len);									\
				(__buffer)->offset += __len;															

#define Buffer_AppendUnchecked(__buffer, __pstr, __len)										\
				FastMemCopy ((__buffer)->offset, __pstr, __len);									\
				(__buffer)->offset += __len;															


#define Buffer_AppendCharUnchecked(__buffer, __chr)												\
				*((__buffer)->offset++) = __chr;																	\

#define Buffer_Reserve(__buffer, __len)																		\
				while ((__buffer)->offset + (__len) > (__buffer)->end)						\
				{																																	\
					Buffer_Realloc((__buffer));																			\
				}																																	\

INLINEFUNCTION static void strreverse(char* begin, char* end)
{
    char aux;
    while (end > begin)
        aux = *end, *end-- = *begin, *begin++ = aux;
}

INLINEFUNCTION void Buffer_AppendIntUnchecked(Buffer *buffer, JSLONG value)
{
    char* wstr;
		JSULONG uvalue = (value < 0) ? -value : value;

		// Reserve space
		//FIXME: 33 seems a bit magic?
		/*
		while(buffer->offset + 33 > buffer->end)
		{
			Buffer_Realloc(buffer);
		}*/

		wstr = buffer->offset;

    
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (uvalue % 10)); while(uvalue /= 10);
    if (value < 0) *wstr++ = '-';
    
    // Reverse string
    strreverse(buffer->offset,wstr - 1);
		buffer->offset += (wstr - 1 - (buffer->offset));
}



INLINEFUNCTION void Buffer_AppendDoubleUnchecked(Buffer *buffer, double fp)
{
	char intPart_reversed[64];
	int i, charCount = 0;
	double fp_int, fp_frac;

	fp_frac = modf(fp,&fp_int); //Separate integer/fractional parts

	while (fp_int > 0) //Convert integer part, if any
	{
		intPart_reversed[charCount++] = '0' + (int)fmod(fp_int,10.0);
		fp_int = floor(fp_int/10.0);
	}

	for (i=0; i<charCount; i++) //Reverse the integer part, if any
		*(buffer->offset++) = intPart_reversed[charCount-i-1];

	*(buffer->offset++) = '.'; //Decimal point

	i = 0;

	while (fp_frac > 0 && i < JSON_DOUBLE_MAX_DECIMALS) //Convert fractional part, if any
	{
		fp_frac*=10;
		fp_frac = modf(fp_frac,&fp_int);
		*(buffer->offset++) = '0' + (int)fp_int;
		i ++;
	}
}

static void Buffer_Realloc (Buffer *buffer)
{
	size_t newSize = buffer->end - buffer->start;
	size_t offset = buffer->offset - buffer->start;
	newSize *= 2;

	if (buffer->heap)
	{
		buffer->start = (char *) buffer->realloc (buffer->start, newSize);
	}
	else
	{
		char *oldStart = buffer->start;
		buffer->heap = 1;
		buffer->start = (char *) buffer->malloc (newSize);
		memcpy (buffer->start, oldStart, offset);
	}
	buffer->offset = buffer->start + offset;
	buffer->end = buffer->start + newSize;

}

static char g_escapeLookup[] = {
	/*				 0x00   0x01   0x02    0x03   0x04   0x05    0x06  0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f */
	/* 0x00 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  'b',  't',  'n',  '\0',  'f',  'r',  '\0',  '\0',
	/* 0x10 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x20 */ '\0',  '\0',  '\"',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x30 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x40 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x50 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\\',  '\0',  '\0',  '\0',
	/* 0x60 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x70 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x80 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x90 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xa0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xb0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xc0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xd0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xe0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
};

static void Buffer_Escape (Buffer *buffer, char *inputOffset, size_t len)
{
	char *inputEnd = inputOffset + len;
	char output;

	while (inputOffset < inputEnd)
	{
		output = g_escapeLookup[(unsigned char) *(inputOffset)];

		if (output == '\0')
		{
				*(buffer->offset++) = *(inputOffset++);
		}
		else
		{
			*(buffer->offset++) = '\\';
			*(buffer->offset++) = output;
			inputOffset ++;
		}
	}
}

static void encode(char *_name, size_t _cbName, int level, JSOBJ obj, JSONObjectEncoder *def, Buffer *buffer, int insideList)
{
	JSTYPEINFO ti;
	size_t szlen;
	const char *name = (insideList || level == 0) ? NULL : _name;

	Buffer_Reserve(buffer, 100 + (_cbName * 2));

	if (name != NULL)
	{
		Buffer_AppendEscapeUnchecked (buffer, name, _cbName);
		Buffer_AppendCharUnchecked (buffer, ':');
		Buffer_AppendCharUnchecked (buffer, ' ');
	}
	
	ti.release = 0;
	def->getType(obj, &ti);

	switch (ti.type)
	{
		case JT_ARRAY:
		{
			int count = 0;
			JSOBJ iterObj;
			def->iterBegin(obj, &ti);

			Buffer_AppendCharUnchecked (buffer, '[');

			while (def->iterNext(obj, &ti))
			{
				if (count > 0)
				{
					Buffer_AppendCharUnchecked (buffer, ',');
					Buffer_AppendCharUnchecked (buffer, ' ');
				}

				iterObj = def->iterGetValue(obj, &ti);
				encode (NULL, 0, level + 1, iterObj, def, buffer, 1);			
				count ++;
			}

			def->iterEnd(obj, &ti);
			Buffer_AppendCharUnchecked (buffer, ']');
			break;
		}

		case JT_OBJECT:
		{
			int count = 0;
			JSOBJ iterObj;
			char *objName;
			def->iterBegin(obj, &ti);

			Buffer_AppendCharUnchecked (buffer, '{');

			while (def->iterNext(obj, &ti))
			{
				if (count > 0)
				{
					Buffer_AppendCharUnchecked (buffer, ',');
					Buffer_AppendCharUnchecked (buffer, ' ');
				}

				iterObj = def->iterGetValue(obj, &ti);
				objName = def->iterGetName(obj, &ti, &szlen);

				encode (objName, szlen, level + 1, iterObj, def, buffer, 0);			
				count ++;
			}

			def->iterEnd(obj, &ti);
			Buffer_AppendCharUnchecked (buffer, '}');
			break;
		}

		case JT_INTEGER:
		{
			JSLONG value;
			def->getValue(obj, &ti, &value, &szlen);
			Buffer_AppendIntUnchecked (buffer, value);
			break;
		}

		case JT_TRUE:
		{
			//Buffer_AppendUnchecked (buffer, "true", 4);
			Buffer_AppendCharUnchecked (buffer, 't');
			Buffer_AppendCharUnchecked (buffer, 'r');
			Buffer_AppendCharUnchecked (buffer, 'u');
			Buffer_AppendCharUnchecked (buffer, 'e');
			break;
		}

		case JT_FALSE:
		{
			//Buffer_AppendUnchecked (buffer, "false", 5);
			Buffer_AppendCharUnchecked (buffer, 'f');
			Buffer_AppendCharUnchecked (buffer, 'a');
			Buffer_AppendCharUnchecked (buffer, 'l');
			Buffer_AppendCharUnchecked (buffer, 's');
			Buffer_AppendCharUnchecked (buffer, 'e');
			break;
		}


		case JT_NULL: 
		{
			//Buffer_AppendUnchecked(buffer, "null", 4);
			Buffer_AppendCharUnchecked (buffer, 'n');
			Buffer_AppendCharUnchecked (buffer, 'u');
			Buffer_AppendCharUnchecked (buffer, 'l');
			Buffer_AppendCharUnchecked (buffer, 'l');
			break;
		}

		case JT_DOUBLE:
		{
			double value;
			def->getValue(obj, &ti, &value, &szlen);
			Buffer_AppendDoubleUnchecked (buffer, value);
			break;
		}

		case JT_UTF8:
		{
			char *value;
			value = (char *) def->getValue(obj, &ti, &value, &szlen);
			Buffer_AppendEscape(buffer, value, szlen);
			break;
		}
	}

	if (ti.release)
	{
		def->releaseValue(&ti);
	}
}

char *JSON_EncodeObject(JSOBJ obj, JSONObjectEncoder *def, char *_buffer, size_t _cbBuffer)
{
	Buffer buffer;

	buffer.malloc = def->malloc ? def->malloc : malloc;
	buffer.free = def->free ? def->free : free;
	buffer.realloc = def->realloc ? def->realloc : realloc;

	if (_buffer == NULL)
	{
		_cbBuffer = 32768;
		buffer.start = (char *) buffer.malloc (_cbBuffer);
		buffer.heap = 1;
	}
	else
	{
		buffer.start = _buffer;
		buffer.heap = 0;
	}

	buffer.end = buffer.start + _cbBuffer;
	buffer.offset = buffer.start;

	encode(NULL, 0, 0, obj, def, &buffer, 0);

	Buffer_Append(&buffer, "\0", 1);
	//return buffer.start;
	return buffer.start;
}