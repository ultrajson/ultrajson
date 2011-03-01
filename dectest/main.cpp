extern "C"
{
#include <ultrajson.h>
}

#include <windows.h>
#include <string>
#include <map>
#include <list>
#include <assert.h>

char indata[] = "[[{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}]], [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}]], [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}]], [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}]], [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}]], [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, [{\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}, {\"username\": \"johndoe\", \"jobs\": [1, 2], \"isAuthorized\": true, \"userId\": 3381293, \"currJob\": null, \"approval\": 31.147099999999998, \"fullname\": \"John Doe the Second\"}]]]";

using namespace std;

class BaseObject
{
private:
	JSTYPES m_type;

public:

	BaseObject (JSTYPES _type)
	{
		m_type = _type;
	}

	virtual ~BaseObject()
	{
	}
	
	JSTYPES getType(void)
	{
		return m_type;
	}

	virtual void print(int level, FILE *file) = 0;

	virtual void iterBegin(JSTYPEINFO *ti)
	{
		assert (false);
	}

	virtual int iterNext(JSTYPEINFO *ti)
	{
		assert (false);
		return 0;
	}

	void iterEnd(JSTYPEINFO *ti)
	{
	}

	virtual JSOBJ iterGetValue(JSTYPEINFO *ti)
	{
		assert (false);
		return NULL;
	}

	virtual char *iterGetName(JSTYPEINFO *ti, size_t *outLen)
	{
		assert (false);
		return NULL;
	}

};

class StringObject : public BaseObject
{
	string m_value;

public:
	StringObject(char *start, char *end) : BaseObject(JT_UTF8)
	{
		m_value = string (start, end - start);
	}

	const string &getValue()
	{
		return m_value;
	}

	void print(int level, FILE *file)
	{
		fprintf (file, "\"%s\"", m_value.c_str ());
	}
};

class IntegerObject : public BaseObject
{
	JSLONG m_value;

public:
	IntegerObject(JSLONG _value) : BaseObject(JT_INTEGER)
	{
		m_value = _value;
	}

	void print(int level, FILE *file)
	{
		fprintf (file, "%ld", m_value);
	}

	JSLONG getValue()
	{
		return m_value;
	}

};

class DoubleObject : public BaseObject
{
	double m_value;

public:
	DoubleObject(double _value) : BaseObject(JT_DOUBLE)
	{
		m_value = _value;
	}

	void print(int level, FILE *file)
	{
		fprintf (file, "%f", m_value);
	}

	double getValue()
	{
		return m_value;
	}


};


class BoolObject : public BaseObject
{
	bool m_value;

public:
	BoolObject(bool _value, JSTYPES _type) : BaseObject(_type)
	{
		m_value = _value;
	}

	void print(int level, FILE *file)
	{
		fprintf (file, "%s", m_value ? "true" : "false");
	}
};

class NullObject : public BaseObject
{
public:
	NullObject() : BaseObject(JT_NULL)
	{
	}

	void print(int level, FILE *file)
	{
		fprintf (file, "null");
	}
};

class MapObject : public BaseObject
{
	map<StringObject *, BaseObject *> m_map;
	map<StringObject *, BaseObject *>::iterator m_iter;
	int m_iterCount;

public:
	MapObject() : BaseObject(JT_OBJECT)
	{
	}

	~MapObject()
	{
		for (map<StringObject *, BaseObject *>::iterator iter = m_map.begin(); iter != m_map.end(); iter ++)
		{
			delete (iter->first);
			delete (iter->second);
		}

	}

	void addKey (StringObject *name, BaseObject *item)
	{
		m_map[name] = item;
	}

	void print(int level, FILE *file)
	{
		int count = 0;

		fprintf (file, "{");

		for (map<StringObject *, BaseObject *>::iterator iter = m_map.begin(); iter != m_map.end(); iter ++)
		{
			if (count > 0)
			{
				fprintf (file, ", ");
			}

			StringObject *key = iter->first;
			BaseObject *obj = iter->second;

			key->print(level, file);
			fprintf (file, ": ");
			obj->print(level + 1, file);
			count ++;
		}

		fprintf (file, "}");

	}

	virtual void iterBegin(JSTYPEINFO *ti)
	{
		m_iter = m_map.begin();
		m_iterCount = 0;
	}

	virtual int iterNext(JSTYPEINFO *ti)
	{
		if (m_iterCount == 0)
		{
			m_iter = m_map.begin();

			if (m_iter == m_map.end())
			{
				return 0;
			}

			m_iterCount ++;
			return 1;
		}

		m_iter ++;

		if (m_iter == m_map.end())
		{
			return 0;
		}
		return 1;
	}

	virtual JSOBJ iterGetValue(JSTYPEINFO *ti)
	{
		return m_iter->second;
	}

	virtual char *iterGetName(JSTYPEINFO *ti, size_t *outLen)
	{
		*outLen = m_iter->first->getValue().size();
		return (char *) m_iter->first->getValue().c_str(); 
	}



};

class ListObject : public BaseObject
{
	list<BaseObject *> m_list;
	list<BaseObject *>::iterator m_iter;
	int m_iterCount;

public:
	ListObject() : BaseObject(JT_ARRAY)
	{
	}

	~ListObject()
	{
		for (list<BaseObject *>::iterator iter = m_list.begin(); iter != m_list.end(); iter ++)
		{
			delete (*iter);
		}
	}

	void addItem(BaseObject *item)
	{
		m_list.push_back(item);
	}

	void print(int level, FILE *file)
	{
		int count = 0;

		fprintf (file, "[");

		for (list<BaseObject *>::iterator iter = m_list.begin(); iter != m_list.end(); iter ++)
		{
			if (count > 0)
			{
				fprintf (file, ", ");
			}
			BaseObject *obj = *iter;
			obj->print(level + 1, file);
			count ++;
		}

		fprintf (file, "]");
	}
	
	virtual void iterBegin(JSTYPEINFO *ti)
	{
		m_iterCount = 0;
	}

	virtual int iterNext(JSTYPEINFO *ti)
	{
		if (m_iterCount == 0)
		{
			m_iter = m_list.begin();

			if (m_iter == m_list.end())
			{
				return 0;
			}

			m_iterCount ++;
			return 1;
		}

		m_iter ++;

		if (m_iter == m_list.end())
		{
			return 0;
		}
		return 1;
	}

	virtual JSOBJ iterGetValue(JSTYPEINFO *ti)
	{
		return *m_iter;
	}
};



JSOBJ Object_newString(char *start, char *end)
{
	return (JSOBJ) new StringObject(start, end);
}

void Object_objectAddKey(JSOBJ obj, JSOBJ name, JSOBJ value)
{
	((MapObject *)obj)->addKey( (StringObject *) name, (BaseObject *) value);
}

void Object_arrayAddItem(JSOBJ obj, JSOBJ value)
{
	((ListObject *)obj)->addItem( (ListObject *) value);
}

JSOBJ Object_newTrue()
{ 
	return (JSOBJ) new BoolObject(true, JT_TRUE);
}

JSOBJ Object_newFalse()
{
	return (JSOBJ) new BoolObject(false, JT_FALSE);
}

JSOBJ Object_newNull()
{
	return (JSOBJ) new NullObject();
}

JSOBJ Object_newObject()
{
	return (JSOBJ) new MapObject();
}

JSOBJ Object_newArray()
{
	return (JSOBJ) new ListObject();
}

JSOBJ Object_newInteger(JSLONG value)
{
	return (JSOBJ) new IntegerObject(value);
}

JSOBJ Object_newDouble(double value)
{ 
	return (JSOBJ) new DoubleObject(value);
}

void Object_getType(JSOBJ obj, JSTYPEINFO *ti)
{
	ti->type = ((BaseObject *)obj)->getType();
}

void *Object_getValue(JSOBJ obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	switch (ti->type)
	{
		case JT_INTEGER:
			*((JSLONG *)outValue) = ((IntegerObject*)obj)->getValue();
			break;
		case JT_DOUBLE:
			*((double *)outValue) = ((DoubleObject*)obj)->getValue();
			break;
		case JT_UTF8:
			*_outLen = ((StringObject *) obj)->getValue().size();
			return (void *) ((StringObject *) obj)->getValue().c_str ();

		default:
			assert(false);

	}

	return NULL;
}

void Object_iterBegin(JSOBJ obj, JSTYPEINFO *ti)
{
	((BaseObject *)obj)->iterBegin(ti);

}

int Object_iterNext(JSOBJ obj, JSTYPEINFO *ti)
{
	return ((BaseObject *)obj)->iterNext(ti);
}

void Object_iterEnd(JSOBJ obj, JSTYPEINFO *ti)
{
	((BaseObject *)obj)->iterEnd(ti);
}

JSOBJ Object_iterGetValue(JSOBJ obj, JSTYPEINFO *ti)
{
	return ((BaseObject *)obj)->iterGetValue(ti);
}

char *Object_iterGetName(JSOBJ obj, JSTYPEINFO *ti, size_t *outLen)
{
	return ((BaseObject *)obj)->iterGetName(ti, outLen);
}

void Object_releaseValue(JSTYPEINFO *ti)
{
}





int main (int argc, char **argv)
{
	BaseObject *obj;
	
	JSONObjectEncoder encoder;
	encoder.getType = Object_getType;
	encoder.getValue = Object_getValue;
	encoder.iterBegin = Object_iterBegin;
	encoder.iterNext = Object_iterNext;
	encoder.iterEnd = Object_iterEnd;
	encoder.iterGetValue = Object_iterGetValue;
	encoder.iterGetName = Object_iterGetName;
	encoder.releaseValue = Object_releaseValue;
	encoder.malloc = malloc;
	encoder.realloc = realloc;
	encoder.free = free;
	encoder.recursionMax = 0;
	encoder.recursionError = 0;

	JSONObjectDecoder decoder;
	decoder.arrayAddItem = Object_arrayAddItem;
	decoder.objectAddKey = Object_objectAddKey;
	decoder.newArray = Object_newArray;
	decoder.newObject = Object_newObject;
	decoder.newNull = Object_newNull;
	decoder.newTrue = Object_newTrue;
	decoder.newFalse = Object_newFalse;
	decoder.newInteger = Object_newInteger;
	decoder.newDouble = Object_newDouble;
	decoder.newString = Object_newString;
	decoder.malloc = malloc;
	decoder.free = free;
	decoder.realloc = realloc;

	char buffer[65536];
	#define N 30000

	char *pindata = (char *) malloc(sizeof(indata) + 256);

	memcpy(pindata, indata, sizeof (indata));

	for (size_t index = sizeof(indata); index < sizeof(indata) + 256; index ++)
	{
		pindata[index] = ']';
	}

	while (true)
	{

		DWORD tsStart = GetTickCount();

		for (int index = 0; index < N; index ++)
		{
			obj = (BaseObject *) JSON_DecodeObject (&decoder, indata, sizeof (indata));

			assert (decoder.errorStr == NULL);
			assert (obj);

			delete obj;
		}

		DWORD tsElapsed = GetTickCount () - tsStart;
		fprintf (stderr, "%s: Per sec %f\n", __FUNCTION__, (float) N / ((float) tsElapsed / 1000.0f));
	}

	getchar();
	return 0;
}
