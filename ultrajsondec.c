#include "ultrajson.h"
#include <math.h>

struct DecoderState
{
	char *start;
	char *end;
	int lastType;
};

JSOBJ decode_any(JSONObjectDecoder *dec, struct DecoderState *ds);

typedef JSOBJ (*PFN_DECODER)(JSONObjectDecoder *dec, struct DecoderState *ds);
PFN_DECODER g_identTable[256] = { NULL }; 

static const double g_pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

static JSOBJ StringToNumeric (JSONObjectDecoder *dec, struct DecoderState *ds, int intNeg, char *intStart, char *intEnd, char *frcStart, char *frcEnd, int expNeg, char *expStart, char *expEnd)
{
	JSLONG intValue = 0;
	double frcValue = 0.0;
	double expValue = 0.0;
	double dblValue = 0.0;
	int decimalCount = 0;

	// Decode integer part

	while (intStart < intEnd)
	{
		intValue = intValue * 10 + ((JSLONG) (((int) (unsigned char) (*intStart++)) - 48));
	}

	if (intNeg)
	{
		intValue *= -1;
	}

//DECODE_FRACTION:

	if (frcStart == NULL)
	{
		dblValue = (double) intValue;
		goto DECODE_EXPONENT;
	}

	// Cap to 9 decimals
	if (frcEnd - frcStart > 9)
	{
		frcEnd -= ( (frcEnd - frcStart) - 9);
	}

	while (frcStart < frcEnd)
	{
		frcValue = frcValue * 10.0 + ((double) (((int) (unsigned char) (*frcStart++)) - 48));
		decimalCount ++;
	}
	frcValue /= g_pow10[decimalCount];

	dblValue = (double) intValue; 
	dblValue += frcValue;

	if (expStart == NULL)
	{
		// Create double from intValue and frcValue. Return
		ds->lastType = JT_DOUBLE;
		return dec->newDouble(dblValue);
	}


DECODE_EXPONENT:

	if (expStart == NULL)
	{
		goto DECODE_INTEGER;
	}

	while (expStart < expEnd)
	{
		expValue = expValue * 10.0 + ((double) (((int) (unsigned char) (*frcStart++)) - 48));
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

JSOBJ decode_numeric (JSONObjectDecoder *dec, struct DecoderState *ds)
{
	char *intStart = NULL;
	char *intEnd = NULL;
	char *frcStart = NULL;
	char *frcEnd = NULL;
	char *expStart = NULL;
	char *expEnd = NULL;
	int intNeg = 0;
	int expNeg = 0;
	
	ds->lastType = JT_INVALID;

	if (*(ds->start) == '-')
	{
		ds->start ++;
		intNeg = 1;
	}
	intStart = ds->start;

	// Scan integer part
	while(ds->start < ds->end)
	{
		if (*(ds->start) >= '0' && *(ds->start) <= '9')
		{
			ds->start ++;
			continue;
		}
		else
		if (*(ds->start) == '.')
		{
			intEnd = ds->start;
			ds->start ++;
			goto DECODE_FRACTION;
		}
		else
		if (*(ds->start) == 'e' || *(ds->start) == 'E')
		{
			intEnd = ds->start;
			ds->start ++;
			goto DECODE_EXPONENT;
		}
		else
		{
			intEnd = ds->start;
			goto DECODE_INTEGER;
		}

		// Unreachable
	}

DECODE_INTEGER:
	return StringToNumeric(dec, ds, intNeg, intStart, intEnd, frcStart, frcEnd, expNeg, expStart, expEnd);

DECODE_FRACTION:

	// Scan fraction part
	frcStart = ds->start;

	while(ds->start < ds->end)
	{
		if (*(ds->start) >= '0' && *(ds->start) <= '9')
		{
			ds->start ++;
			continue;
		}
		else
		if (*(ds->start) == 'e' || *(ds->start) == 'E')
		{
			frcEnd = ds->start;
			ds->start ++;
			goto DECODE_EXPONENT;
		}
		else
		{
			break;
		}
	}

	return StringToNumeric(dec, ds, intNeg, intStart, intEnd, frcStart, frcEnd, expNeg, expStart, expEnd);

DECODE_EXPONENT:
	expStart = ds->start;

	if (*(ds->start) == '-')
	{
		expNeg = 1;
		ds->start ++;
	}
	else
	if (*(ds->start) == '+')
	{
		expNeg = 0;
		ds->start ++;
	}

	while(ds->start < ds->end)
	{
		if (*(ds->start) >= '0' && *(ds->start) <= '9')
		{
			ds->start ++;
			continue;
		}
		else
		{
			expEnd = ds->start;
			break;
		}
	}

	return StringToNumeric(dec, ds, intNeg, intStart, intEnd, frcStart, frcEnd, expNeg, expStart, expEnd);
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
		dec->arrayAddItem (newObj, itemValue);

		// Scan for end of object declaration
		while (ds->start < ds->end)
		{
			if (*(ds->start) == ',')
			{
				ds->start ++;
				break;
			}
			else
			if (*(ds->start) == ']')
			{
				ds->start ++;
				ds->lastType = JT_ARRAY;
				return newObj;
			}

			// Skipping over white space and shits
			ds->start ++;
		}
	}

	fprintf (stderr, "%s: Unmatched ] when parsing object\n", __FUNCTION__);
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
			fprintf (stderr, "%s: No : found when parsing object key\n", __FUNCTION__);
			return NULL;
		}
		
		itemValue = decode_any(dec, ds);
		dec->objectAddKey (newObj, itemName, itemValue);

		// Scan for end of object declaration
		while (ds->start < ds->end)
		{
			if (*(ds->start) == ',')
			{
				ds->start ++;
				break;
			}
			else
			if (*(ds->start) == '}')
			{
				ds->start ++;
				ds->lastType = JT_OBJECT;
				return newObj;
			}

			// Skipping over white space and shits
			ds->start ++;
		}
	}

	fprintf (stderr, "%s: Unmatched } when parsing object\n", __FUNCTION__);
	return NULL;
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
			fprintf (stderr, "%s: Unexpected character found\n", __FUNCTION__);
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

	for (index = 0; index < 0x21; index ++)
	{
		g_identTable[index] = (PFN_DECODER) 1; 
	}


	/*
	JT_UTF8 "x"
	JT_INTEGER 2312383120312
	JT_FLOAT 321321.321321 or 231e/E23190
	JT_TRUE true
	JT_FALSE false
	JT_NULL null
	JS_ARRAY [ x ]
	JS_OBJECT { "", x }	
	*/

	return decode_any (dec, &ds);
	


}
