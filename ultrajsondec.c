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
*/

#include "ultrajson.h"
#include <math.h>
#include <assert.h>
#include <string.h>

struct DecoderState
{
	char *start;
	char *end;
	char *escStart;
	char *escEnd;
	int escHeap;
	int lastType;
};

INLINEFUNCTION JSOBJ decode_any(JSONObjectDecoder *dec, struct DecoderState *ds);

typedef JSOBJ (*PFN_DECODER)(JSONObjectDecoder *dec, struct DecoderState *ds);
PFN_DECODER g_identTable[256] = { NULL }; 

static const double g_pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};


INLINEFUNCTION double createDouble(double intValue, double frcValue, int frcDecimalCount)
{
	return intValue + (frcValue / g_pow10[frcDecimalCount]);
}

JSOBJ decode_numeric (JSONObjectDecoder *dec, struct DecoderState *ds)
{
	int intNeg = 1;
	int expNeg = 1;
	int decimalCount = 0;
	JSLONG intValue;
	double frcValue = 0.0;
	double expValue;
	
	ds->lastType = JT_INVALID;

	// Go one back since the scanner ate one
	ds->start --;

	if (*(ds->start) == '-')
	{
		ds->start ++;
		intNeg = -1;
	}

	// Scan integer part

	intValue = 0;

	while(*(ds->start) != '\0')
	{
		if (*(ds->start) >= '0' && *(ds->start) <= '9')
		{
			intValue = intValue * 10 + ((JSLONG) (((int) (unsigned char) (*ds->start)) - 48));
			ds->start ++;
			continue;
		}
		else
		if (*(ds->start) == '.')
		{
			ds->start ++;
			goto DECODE_FRACTION;
		}
		else
		if (*(ds->start) == 'e' || *(ds->start) == 'E')
		{
			ds->start ++;
			goto DECODE_EXPONENT;
		}
		else
		{
			goto DECODE_INTEGER;
		}

		// Unreachable
	}

DECODE_INTEGER:

	ds->lastType = JT_INTEGER;
	return dec->newInteger(intValue * intNeg);

DECODE_FRACTION:

	// Scan fraction part
	frcValue = 0.0;
	while(*(ds->start) != '\0')
	{
		if (*(ds->start) >= '0' && *(ds->start) <= '9')
		{
			if (decimalCount < JSON_DOUBLE_MAX_DECIMALS)
			{
				frcValue = frcValue * 10.0 + ((double) (((int) (unsigned char) (*ds->start)) - 48));
				decimalCount ++;
			}
			ds->start ++;

			continue;
		}
		else
		if (*(ds->start) == 'e' || *(ds->start) == 'E')
		{
			ds->start ++;
			goto DECODE_EXPONENT;
		}
		else
		{
			break;
		}
	}

	ds->lastType = JT_DOUBLE;
	return dec->newDouble (createDouble( (double) intValue * intNeg, frcValue, decimalCount));

DECODE_EXPONENT:
	if (*(ds->start) == '-')
	{
		expNeg = -1;
		ds->start ++;
	}
	else
	if (*(ds->start) == '+')
	{
		expNeg = +1;
		ds->start ++;
	}

	expValue = 0.0;

	while(*(ds->start) != '\0')
	{
		if (*(ds->start) >= '0' && *(ds->start) <= '9')
		{
			expValue = expValue * 10.0 + ((double) (((int) (unsigned char) (*ds->start)) - 48));
			ds->start ++;
			continue;
		}
		else
		{
			break;
		}
	}

	ds->lastType = JT_DOUBLE;
	return dec->newDouble (createDouble( (double) intValue * intNeg, frcValue, decimalCount) * pow(10.0, expValue));
}

JSOBJ decode_true (JSONObjectDecoder *dec, struct DecoderState *ds)
{
	ds->lastType = JT_INVALID;

	if (*(ds->start++) != 'r')
		return NULL;
	if (*(ds->start++) != 'u')
		return NULL;
	if (*(ds->start++) != 'e')
		return NULL;

	ds->lastType = JT_TRUE;
	return dec->newTrue();
}

JSOBJ decode_false (JSONObjectDecoder *dec, struct DecoderState *ds)
{
	ds->lastType = JT_INVALID;

	if (*(ds->start++) != 'a')
		return NULL;
	if (*(ds->start++) != 'l')
		return NULL;
	if (*(ds->start++) != 's')
		return NULL;
	if (*(ds->start++) != 'e')
		return NULL;

	ds->lastType = JT_FALSE;
	return dec->newFalse();
}


JSOBJ decode_null (JSONObjectDecoder *dec, struct DecoderState *ds)
{
	ds->lastType = JT_INVALID;

	if (*(ds->start++) != 'u')
		return NULL;
	if (*(ds->start++) != 'l')
		return NULL;
	if (*(ds->start++) != 'l')
		return NULL;

	ds->lastType = JT_NULL;
	return dec->newNull();
}


static char g_unescapeLookup[] = {
	/*				 0x00   0x01   0x02    0x03   0x04   0x05    0x06  0x07   0x08   0x09   0x0a   0x0b   0x0c   0x0d   0x0e   0x0f */
	/* 0x00 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x10 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x20 */ '\0',  '\0',  '\"',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x30 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x40 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x50 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x60 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x70 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x80 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0x90 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xa0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xb0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xc0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xd0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xe0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
	/* 0xf0 */ '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',  '\0',
};

JSOBJ decode_string (JSONObjectDecoder *dec, struct DecoderState *ds)
{
	char *escOffset;
	char chr;
	size_t escLen = (ds->escEnd - ds->escStart);
	ds->lastType = JT_INVALID;

	if ( (ds->end - ds->start) > escLen)
	{
		size_t newSize = escLen * 2;

		if (ds->escHeap)
		{
			ds->escStart = (char *) dec->realloc (ds->escStart, escLen);
		}
		else
		{
			char *oldStart = ds->escStart;
			ds->escHeap = 1;
			ds->escStart = (char *) dec->malloc (newSize);
			memcpy (ds->escStart, oldStart, escLen);
		}

		ds->escEnd = ds->escStart + newSize;
	}

	escOffset = ds->escStart;

	while (*(ds->start) != '\0')
	{
		chr = (*ds->start++);

		if (chr == '\"')
		{
			ds->lastType = JT_UTF8;
			return dec->newString(ds->escStart, escOffset);
		}
		else
		{
			if (chr == '\\')
			{
				if (ds->start >= ds->end)
				{
					fprintf (stderr, "%s: Unterminated escape sequencein string literal\n", __FUNCTION__);
					return NULL;
				}

				chr = g_unescapeLookup[ (unsigned char) (*ds->start++)];

				if (chr == '\0')
				{
					fprintf (stderr, "%s: Unrecognized escape sequeunce in string literal\n", __FUNCTION__);
					return NULL;
				}

			}
		}

		*(escOffset++) = chr;
	}
	fprintf (stderr, "%s: Unmatched \" when decoding string\n", __FUNCTION__);
	return NULL;
}

INLINEFUNCTION JSOBJ decode_array(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	JSOBJ itemValue;
	JSOBJ newObj = dec->newArray();
	ds->lastType = JT_INVALID;

	while (*(ds->start) != '\0')
	{
		itemValue = decode_any(dec, ds);

		if (itemValue == NULL)
		{
			ds->lastType = JT_ARRAY;
			return newObj;
		}

		dec->arrayAddItem (newObj, itemValue);
	}

	fprintf (stderr, "%s: Unmatched ']' when parsing array\n", __FUNCTION__);
	return NULL;
}



INLINEFUNCTION JSOBJ decode_object(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	JSOBJ itemName;
	JSOBJ itemValue;
	JSOBJ newObj = dec->newObject();
	ds->lastType = JT_INVALID;

	while (*(ds->start) != '\0')
	{
		itemName = decode_any(dec, ds);

		if (itemName == NULL)
		{
			ds->lastType = JT_OBJECT;
			return newObj;
		}


		if (itemName)
		{
			if (ds->lastType != JT_UTF8)
			{
				fprintf (stderr, "%s: Object is expected to have key value as string\n", __FUNCTION__);
				return NULL;
			}

			// Expect ':'
			while (*(ds->start) != '\0')
			{
				if (*(ds->start++) == ':')
					break;
			}

			if (ds->start == ds->end)
			{
				fprintf (stderr, "%s: No ':' found when parsing object key\n", __FUNCTION__);
				return NULL;
			}

			itemValue = decode_any(dec, ds);
			dec->objectAddKey (newObj, itemName, itemValue);
		}
	}

	fprintf (stderr, "%s: Unmatched } when parsing object\n", __FUNCTION__);
	return NULL;
}


JSOBJ decode_array_begin(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	return decode_array(dec, ds);
}

JSOBJ decode_array_end(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	return NULL;
}

JSOBJ decode_object_begin(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	return decode_object(dec, ds);
}

JSOBJ decode_object_end(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	return NULL;
}




JSOBJ decode_item_separator(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	//FIXME: Validate that we are inside array or object here
	return decode_any(dec, ds);
}


INLINEFUNCTION JSOBJ decode_any(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	PFN_DECODER pfnDecoder;
	ds->lastType = JT_INVALID;

	//while (*(ds->start) != '\0')
	
LOOP_FLAG:
	pfnDecoder = g_identTable[(unsigned char) *(ds->start++)];

	// 1 means white space
	switch ( (size_t) pfnDecoder)
	{
	case 1:
		goto LOOP_FLAG;

	case 0:
		return NULL;

	default:
		return pfnDecoder(dec, ds);
	}

	// Unreachable
	//return NULL;
}


JSOBJ JSON_DecodeObject(JSONObjectDecoder *dec, const char *buffer, size_t cbBuffer)
{
	static int s_once = 1;

	struct DecoderState ds;
	int index;
	char escBuffer[65536];
	JSOBJ ret;
	

	ds.start = (char *) buffer;
	ds.end = ds.start + cbBuffer;
	ds.lastType = JT_INVALID;
	ds.escStart = escBuffer;
	ds.escEnd = ds.escStart + sizeof(escBuffer);
	ds.escHeap = 0;

	if (s_once)
	{
		//FIXME: Move to static initialization instead
		g_identTable['\"'] = decode_string;
		g_identTable['0'] = decode_numeric;
		g_identTable['1'] = decode_numeric;
		g_identTable['2'] = decode_numeric;
		g_identTable['3'] = decode_numeric;
		g_identTable['4'] = decode_numeric;
		g_identTable['5'] = decode_numeric;
		g_identTable['6'] = decode_numeric;
		g_identTable['7'] = decode_numeric;
		g_identTable['8'] = decode_numeric;
		g_identTable['9'] = decode_numeric;
		g_identTable['0'] = decode_numeric;
		g_identTable['-'] = decode_numeric;
		g_identTable['['] = decode_array;
		g_identTable['{'] = decode_object; 
		g_identTable['t'] = decode_true;
		g_identTable['f'] = decode_false;
		g_identTable['n'] = decode_null; 
		g_identTable['}'] = decode_object_end; 
		g_identTable[']'] = decode_array_end; 
		g_identTable[','] = decode_item_separator; 
		for (index = 0; index < 33; index ++)
		{
			g_identTable[index] = (PFN_DECODER) 1; 
		}

		for (index = 0; index < 256; index ++)
		{
			g_unescapeLookup[index] = '\0';
		}

		g_unescapeLookup['\"'] = '\"';
		g_unescapeLookup['/'] = '/';
		g_unescapeLookup['b'] = '\b';
		g_unescapeLookup['f'] = '\f';
		g_unescapeLookup['n'] = '\n';
		g_unescapeLookup['r'] = '\r';
		g_unescapeLookup['t'] = '\t';

		//FIXME: Implement unicode decode here



		s_once = 0;
	}

	ret = decode_any (dec, &ds);
	
	if (ds.escHeap)
	{
		dec->free(ds.escStart);
	}

	return ret;
}
