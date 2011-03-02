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

#include "ultrajson.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

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

void Buffer_Realloc (Buffer *buffer);

//FIXME: Simply remove this macro
#define FastMemCopy memcpy

//FIXME: Perhaps a bit premature. Let the compilre handle this instead and make these into functions
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


/**
 * Powers of 10
 * 10^0 to 10^9
 */
static const double g_pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

void strreverse(char* begin, char* end)
{
    char aux;
    while (end > begin)
        aux = *end, *end-- = *begin, *begin++ = aux;
}

void Buffer_AppendIntUnchecked(Buffer *buffer, JSLONG value)
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
		buffer->offset += (wstr - (buffer->offset));
}

/*
FIXME:
Pass prec as a configurable parameter in a shared encoder context instead */

void Buffer_AppendDoubleUnchecked(Buffer *buffer, double value, int prec)
{
    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);
    int count;
    double diff = 0.0;
		char* str = buffer->offset;
    char* wstr = str;
    int whole;
    double tmp;
    uint32_t frac;
		int neg;
		
		/* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (! (value == value)) {
				//FIXME: Don't return nan it's not supported by JSON
        str[0] = 'n'; str[1] = 'a'; str[2] = 'n'; str[3] = '\0';
        return;
    }

    if (prec < 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    neg = 0;
    if (value < 0) {
        neg = 1;
        value = -value;
    }

    whole = (int) value;
    tmp = (value - whole) * g_pow10[prec];
    frac = (uint32_t)(tmp);
    diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        /* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
        if (frac >= g_pow10[prec]) {
            frac = 0;
            ++whole;
        }
    } else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
        /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
        ++frac;
    }

    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
      normal printf behavior is to print EVERY whole number digit
      which can be 100s of characters overflowing your buffers == bad
    */
    if (value > thres_max) {
				//FIXME: sprintf might code Nan or InF here, it's not in the standard
				buffer->offset += sprintf(str, "%e", neg ? -value : value);
        return;
    }

    if (prec == 0) {
        diff = value - whole;
        if (diff > 0.5) {
            /* greater than 0.5, round up, e.g. 1.6 -> 2 */
            ++whole;
        } else if (diff == 0.5 && (whole & 1)) {
            /* exactly 0.5 and ODD, then round up */
            /* 1.5 -> 2, but 2.5 -> 2 */
            ++whole;
        }

        //vvvvvvvvvvvvvvvvvvv  Diff from modp_dto2
    } 
		else if (frac) { 

			count = prec;
        // now do fractional part, as an unsigned number
        // we know it is not 0 but we can have leading zeros, these
        // should be removed
        while (!(frac % 10)) {
            --count;
            frac /= 10;
        }
        //^^^^^^^^^^^^^^^^^^^  Diff from modp_dto2

        // now do fractional part, as an unsigned number
        do {
            --count;
            *wstr++ = (char)(48 + (frac % 10));
        } while (frac /= 10);
        // add extra 0s
        while (count-- > 0) *wstr++ = '0';
        // add decimal
        *wstr++ = '.';
    }

    // do whole part
    // Take care of sign
    // Conversion. Number is reversed.
    do *wstr++ = (char)(48 + (whole % 10)); while (whole /= 10);
    if (neg) {
        *wstr++ = '-';
    }
    strreverse(str, wstr-1);
		buffer->offset += (wstr - (buffer->offset));
}


/*
FIXME: Keep track of how big these get across several encoder calls and try to make an estimate
Thay way we won't run our head into the wall each call */

void Buffer_Realloc (Buffer *buffer)
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

void Buffer_Escape (Buffer *buffer, char *inputOffset, size_t len)
{
	char *inputEnd = inputOffset + len;
	char output;

	//FIXME: Encode '\uXXXX' here
	while (1)
	{
		switch (*inputOffset)
		{
			case '\0': return;
			case '\"': *(buffer->offset++) = '\\'; *(buffer->offset++) = '\"';break;
			case '\\': *(buffer->offset++) = '\\'; *(buffer->offset++) = '\\';break;
			
			/*
			NOTE: The RFC says escape solidus but none of the reference encoders does so.
			We don't do it either now ;)
			case '/': *(buffer->offset++) = '\\'; *(buffer->offset++) = '/';break;
			*/
			case '\b': *(buffer->offset++) = '\\'; *(buffer->offset++) = 'b';break;
			case '\f': *(buffer->offset++) = '\\'; *(buffer->offset++) = 'f';break;
			case '\n': *(buffer->offset++) = '\\'; *(buffer->offset++) = 'n';break;
			case '\r': *(buffer->offset++) = '\\'; *(buffer->offset++) = 'r';break;
			case '\t': *(buffer->offset++) = '\\'; *(buffer->offset++) = 't';break;
			default: (*buffer->offset++) = *(inputOffset); break;
		}
		inputOffset ++;
	}
}

/*
FIXME:
Merge Buffer, JSONObjectEncoder and JSONTypeContext into one common type including _name, _cbName, level and obj
*/

/*
FIXME:
Handle functions actually returning NULL here */

/*
FIXME:
Perhaps implement recursion detection */

void encode(char *_name, size_t _cbName, int level, JSOBJ obj, JSONObjectEncoder *def, Buffer *buffer, int insideList)
{
	JSONTypeContext ti;
	size_t szlen;
	const char *name = (insideList || level == 0) ? NULL : _name;

	if (level > def->recursionMax)
	{
		def->recursionError = 1;
		return;
	}

	/*
	This reservation must hold 

	length of _name as encoded worst case +
	maxLength of double to string OR maxLength of JSLONG to string
	*/

	Buffer_Reserve(buffer, 256 + (_cbName * 2));

	if (name != NULL)
	{
		Buffer_AppendEscapeUnchecked (buffer, name, _cbName);
		Buffer_AppendCharUnchecked (buffer, ':');
#ifndef JSON_NO_EXTRA_WHITESPACE
		Buffer_AppendCharUnchecked (buffer, ' ');
#endif
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
#ifndef JSON_NO_EXTRA_WHITESPACE
					Buffer_AppendCharUnchecked (buffer, ' ');
#endif
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
#ifndef JSON_NO_EXTRA_WHITESPACE
					Buffer_AppendCharUnchecked (buffer, ' ');
#endif
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
			Buffer_AppendDoubleUnchecked (buffer, value, JSON_DOUBLE_MAX_DECIMALS);
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



//FIXME: Make JSON_MAX_RECURSION_DEPTH and depending on performance JSON_NO_EXTRA_WHITESPACE as configuration options
char *JSON_EncodeObject(JSOBJ obj, JSONObjectEncoder *def, char *_buffer, size_t _cbBuffer)
{
	Buffer buffer;

	buffer.malloc = def->malloc ? def->malloc : malloc;
	buffer.free = def->free ? def->free : free;
	buffer.realloc = def->realloc ? def->realloc : realloc;

	def->recursionError = 0;

	if (def->recursionMax < 1)
	{
		def->recursionMax = JSON_MAX_RECURSION_DEPTH;
	}

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
