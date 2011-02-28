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
#include <math.h>
#include <assert.h>

struct DecoderState
{
	char *start;
	char *end;
	int lastType;

	int cObjBegin;
	int cObjEnd;
	int cArrBegin;
	int cArrEnd;

};

JSOBJ decode_any(JSONObjectDecoder *dec, struct DecoderState *ds);

typedef JSOBJ (*PFN_DECODER)(JSONObjectDecoder *dec, struct DecoderState *ds);
PFN_DECODER g_identTable[256] = { NULL }; 

static const double g_pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

/*
static JSOBJ StringToNumeric (JSONObjectDecoder *dec, struct DecoderState *ds, int intNeg, JSLONG intValue, int hasFrc, int decimalCount, double frcValue, int hasExp, double expValue, int expNeg)
{
	double dblValue;
	// Decode integer part

	if (intNeg)
	{
		intValue *= -1;
	}

	if (!hasFrc)
	{
		dblValue = (double) intValue;
		goto DECODE_EXPONENT;
	}

	frcValue /= g_pow10[decimalCount];

	dblValue = (double) intValue; 
	dblValue += frcValue;

	if (!hasExp)
	{
		// Create double from intValue and frcValue. Return
		ds->lastType = JT_DOUBLE;
		return dec->newDouble(dblValue);
	}

DECODE_EXPONENT:

	if (!hasExp)
	{
		goto DECODE_INTEGER;
	}

	if (expNeg)
	{
		expValue *= -1.0;
	}

	dblValue *= pow(10.0, expValue);
	ds->lastType = JT_DOUBLE;
	return dec->newDouble(dblValue);

DECODE_INTEGER:
	ds->lastType = JT_INTEGER;
	return dec->newInteger(intValue);
}
*/



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

	while(ds->start < ds->end)
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
	while(ds->start < ds->end)
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

	while(ds->start < ds->end)
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


JSOBJ decode_string (JSONObjectDecoder *dec, struct DecoderState *ds)
{
	char *strStart = ds->start;
	ds->lastType = JT_INVALID;

	while (ds->start < ds->end)
	{
		//FIXME: Handle escaping here

		if (*(ds->start++) == '\"')
		{
			ds->lastType = JT_UTF8;
			return dec->newString(strStart, ds->start - 1);
		}
	}
	fprintf (stderr, "%s: Unmatched \" when decoding string\n", __FUNCTION__);
	return NULL;
}

JSOBJ decode_array(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	JSOBJ itemValue;
	JSOBJ newObj = dec->newArray();
	ds->lastType = JT_INVALID;

	while (ds->start < ds->end)
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



JSOBJ decode_object(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	JSOBJ itemName;
	JSOBJ itemValue;
	JSOBJ newObj = dec->newObject();
	ds->lastType = JT_INVALID;

	while (ds->start < ds->end)
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
			while (ds->start < ds->end)
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
	ds->cArrBegin ++;
	return decode_array(dec, ds);
}

JSOBJ decode_array_end(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	ds->cArrEnd ++;
	assert (ds->cArrBegin >= ds->cArrEnd);
	return NULL;
}

JSOBJ decode_object_begin(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	ds->cObjBegin ++;
	return decode_object(dec, ds);
}

JSOBJ decode_object_end(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	ds->cObjEnd ++;

	assert (ds->cObjBegin >= ds->cObjEnd);

	return NULL;
}




JSOBJ decode_item_separator(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	//FIXME: Validate that we are inside array or object here
	return decode_any(dec, ds);
}


JSOBJ decode_any(JSONObjectDecoder *dec, struct DecoderState *ds)
{
	PFN_DECODER pfnDecoder;
	ds->lastType = JT_INVALID;

	while (ds->start < ds->end)
	{
		pfnDecoder = g_identTable[(unsigned char) *(ds->start++)];

		// 1 means white space
		if (pfnDecoder == (PFN_DECODER) 1)
		{
			continue;
		}
		else
		if (pfnDecoder == NULL)
		{
			//fprintf (stderr, "%s: Unexpected character found\n", __FUNCTION__);
			return NULL;
		}

		return pfnDecoder(dec, ds);
	}

	return NULL;
}


JSOBJ JSON_DecodeObject(JSONObjectDecoder *dec, const char *buffer, size_t cbBuffer)
{
	struct DecoderState ds;
	int index;

	ds.start = (char *) buffer;
	ds.end = ds.start + cbBuffer;
	ds.lastType = JT_INVALID;

	ds.cObjBegin = ds.cObjEnd = ds.cArrBegin = ds.cArrEnd = 0;

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
	g_identTable['['] = decode_array_begin;
	g_identTable['{'] = decode_object_begin; 
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

	return decode_any (dec, &ds);
	


}
