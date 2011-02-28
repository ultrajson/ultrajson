#include <Python.h>
#include <datetime.h>
#include "../ultrajson.h"

static PyObject* meth_timegm;
static PyObject* mod_calendar;

enum PRIVATE
{
	PRV_CONV_FUNC,					// Function pointer to converter function
	PRV_CONV_NEWOBJ,				// Any new PyObject created by converter function that should be released by releaseValue
	PRV_ITER_BEGIN_FUNC,		// Function pointer to iterBegin for specific type
	PRV_ITER_END_FUNC,			// Function pointer to iterEnd for specific type
	PRV_ITER_NEXT_FUNC,			// Function pointer to iterNext for specific type
	PRV_ITER_GETVALUE_FUNC,
	PRV_ITER_GETNAME_FUNC,
	PRV_ITER_INDEX,					// Index in the iteration list
	PRV_ITER_SIZE,					// Size of the iteration list
	PRV_ITER_ITEM,					// Current iter item
	PRV_ITER_ITEM_NAME,			// Name of iter item
	PRV_ITER_ITEM_VALUE,		// Value of iteritem
	PRV_ITER_DICTITEMS,
	PRV_ITER_DICTOBJ,
	PRV_ITER_ATTRLIST,
};

struct PyDictIterState
{
	PyObject *keys;
	size_t i;
	size_t sz;
};


//#define PRINTMARK() fprintf(stderr, "%s: MARK(%d)\n", __FILE__, __LINE__)		
#define PRINTMARK() 		

void initObjToJSON()
{
	//FIXME: DECREF on these?
	PyDateTime_IMPORT;

	/*
	FIXME: Find the direct function pointer here instead and use it when time conversion is performed */

	meth_timegm = PyString_FromString("timegm");
	mod_calendar = PyImport_ImportModule("calendar");

	Py_INCREF(mod_calendar);

}


typedef void *(*PFN_PyTypeToJSON)(JSOBJ obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen);

static void *PyNoneToNULL(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	return NULL;
}

static void *PyBoolToBOOLEAN(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;
	//FIXME: Perhaps we can use the PyBoolObject->ival directly here?
	*((int *) outValue) = (obj == Py_True) ? 1 : 0;
	return NULL;
}

static void *PyIntToINTEGER(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;
	*((JSLONG *) outValue) = PyInt_AS_LONG (obj);
	return NULL;
}

static void *PyLongToINTEGER(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;
	*((JSLONG *) outValue) = PyLong_AsLongLong (obj);
	return NULL;
}

static void *PyLongToUTF8(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;
	PyObject *newobj;
	char *ret;
	PRINTMARK();


	newobj = PyObject_Str(obj);

	if (newobj)
	{
		ti->prv[PRV_CONV_NEWOBJ] = newobj;
		ti->release = 1;
		*_outLen = PyString_GET_SIZE(newobj);
		//fprintf (stderr, "%s: Object %p needs release!\n", __FUNCTION__, newobj);
		ret = PyString_AS_STRING(newobj);
	}
	else
	{
		ret = "";
		*_outLen = 0;
	}
	return ret;
}

static void *PyFloatToDOUBLE(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;
	*((double *) outValue) = PyFloat_AS_DOUBLE (obj);
	return NULL;
}

static void *PyStringToUTF8(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;
	*_outLen = PyString_GET_SIZE(obj);
	return PyString_AS_STRING(obj);
}

static void *PyUnicodeToUTF8(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;
	PyObject *newObj = PyUnicode_EncodeUTF8 (PyUnicode_AS_UNICODE(obj), PyUnicode_GET_SIZE(obj), NULL);
	ti->release = 1;
	ti->prv[PRV_CONV_NEWOBJ] = (void *) newObj;

	*_outLen = PyString_GET_SIZE(newObj);
	return PyString_AS_STRING(newObj);
}

static void *PyDateTimeToINTEGER(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;

	PyObject* timetuple = PyObject_CallMethod(obj, "utctimetuple", NULL);
	PyObject* unixTimestamp = PyObject_CallMethodObjArgs(mod_calendar, meth_timegm, timetuple, NULL);
	
	*( (JSLONG *) outValue) = PyLong_AsLongLong (unixTimestamp);
	Py_DECREF(timetuple);
	Py_DECREF(unixTimestamp);
	return NULL;
}

static void *PyDateToINTEGER(JSOBJ _obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	PyObject *obj = (PyObject *) _obj;

	PyObject* timetuple = PyTuple_New(6);
	PyObject* year = PyObject_GetAttrString(obj, "year");
	PyObject* month = PyObject_GetAttrString(obj, "month");
	PyObject* day = PyObject_GetAttrString(obj, "day");
	PyObject* unixTimestamp;

	PyTuple_SET_ITEM(timetuple, 0, year);
	PyTuple_SET_ITEM(timetuple, 1, month);
	PyTuple_SET_ITEM(timetuple, 2, day);
	PyTuple_SET_ITEM(timetuple, 3, PyInt_FromLong(0));
	PyTuple_SET_ITEM(timetuple, 4, PyInt_FromLong(0));
	PyTuple_SET_ITEM(timetuple, 5, PyInt_FromLong(0));

	unixTimestamp = PyObject_CallMethodObjArgs(mod_calendar, meth_timegm, timetuple, NULL);

	*( (JSLONG *) outValue) = PyLong_AsLongLong (unixTimestamp);
	Py_DECREF(timetuple);
	Py_DECREF(unixTimestamp);
	return NULL;
}

//=============================================================================
// Tuple iteration functions 
// itemValue is borrowed reference, no ref counting
//=============================================================================
void Tuple_iterBegin(JSOBJ obj, JSTYPEINFO *ti)
{
	ti->prv[PRV_ITER_INDEX] = (void *) 0;
	ti->prv[PRV_ITER_SIZE] = (void *) PyTuple_GET_SIZE( (PyObject *) obj);
	ti->prv[PRV_ITER_ITEM_VALUE] = NULL;
}

int Tuple_iterNext(JSOBJ obj, JSTYPEINFO *ti)
{
	size_t i = (size_t) ti->prv[PRV_ITER_INDEX];
	size_t sz = (size_t) ti->prv[PRV_ITER_SIZE];
	PyObject *item;

	if (i >= sz)
	{
		return 0;
	}

	item = PyTuple_GET_ITEM (obj, i);

	ti->prv[PRV_ITER_ITEM_VALUE] = item;
	i ++;
	ti->prv[PRV_ITER_INDEX] = (void *) i;
	return 1;
}

void Tuple_iterEnd(JSOBJ obj, JSTYPEINFO *ti)
{
}

JSOBJ Tuple_iterGetValue(JSOBJ obj, JSTYPEINFO *ti)
{
	PyObject *curr = (PyObject *) ti->prv[PRV_ITER_ITEM_VALUE];
	return curr;
}

char *Tuple_iterGetName(JSOBJ obj, JSTYPEINFO *ti, size_t *outLen)
{
	return NULL;
}

//=============================================================================
// Dir iteration functions 
// itemName ref is borrowed from PyObject_Dir (attrList). No refcount
// itemValue ref is from PyObject_GetAttr. Ref counted
//=============================================================================
void Dir_iterBegin(JSOBJ obj, JSTYPEINFO *ti)
{
	PyObject* attrList = PyObject_Dir(obj);
	ti->prv[PRV_ITER_INDEX] = (void *) 0;
	ti->prv[PRV_ITER_SIZE] = (void *) PyList_GET_SIZE(attrList);
	ti->prv[PRV_ITER_ITEM] = NULL;
	ti->prv[PRV_ITER_ITEM_NAME] = NULL;
	ti->prv[PRV_ITER_ITEM_VALUE] = NULL;
	ti->prv[PRV_ITER_ATTRLIST] = attrList;
	PRINTMARK();
}

void Dir_iterEnd(JSOBJ obj, JSTYPEINFO *ti)
{
	PyObject *itemValue = (PyObject *) ti->prv[PRV_ITER_ITEM_VALUE];

	if (itemValue)
	{
		Py_DECREF(itemValue);
		ti->prv[PRV_ITER_ITEM_VALUE] = itemValue = NULL;
	}

	Py_DECREF( (PyObject *) ti->prv[PRV_ITER_ATTRLIST]);
	PRINTMARK();
}

int Dir_iterNext(JSOBJ _obj, JSTYPEINFO *ti)
{
	PyObject *obj = (PyObject *) _obj;
	size_t i = (size_t) ti->prv[PRV_ITER_INDEX];
	size_t sz = (size_t) ti->prv[PRV_ITER_SIZE];
	PyObject *attrList = (PyObject *) ti->prv[PRV_ITER_ATTRLIST];
	PyObject *itemName = NULL;
	PyObject *itemValue = (PyObject *) ti->prv[PRV_ITER_ITEM_VALUE];

	if (itemValue)
	{
		Py_DECREF(itemValue);
		ti->prv[PRV_ITER_ITEM_VALUE] = itemValue = NULL;
	}

	//fprintf (stderr, "%s: ti=%p obj=%p, i=%u, sz=%u\n", __FUNCTION__, ti, _obj, i, sz);

	for (; i < sz; i ++)
	{
		PyObject* attr = PyList_GET_ITEM(attrList, i);
		char* attrStr = PyString_AS_STRING(attr);

		if (attrStr[0] == '_')
		{
			PRINTMARK();
			continue;
		}

		itemValue = PyObject_GetAttr(obj, attr);
		if (itemValue == NULL)
		{
			PyErr_Clear();
			PRINTMARK();
			continue;
		}

		if (PyCallable_Check(itemValue))
		{
			Py_DECREF(itemValue);
			PRINTMARK();
			continue;
		}

		PRINTMARK();
		itemName = attr;
		break;
	}

	if (itemName == NULL)
	{
		i = sz;
		ti->prv[PRV_ITER_ITEM_VALUE] = NULL;
		return 0;
	}

	ti->prv[PRV_ITER_ITEM_NAME] = itemName;
	ti->prv[PRV_ITER_ITEM_VALUE] = itemValue;
	ti->prv[PRV_ITER_INDEX] = (void *) (i + 1);
	PRINTMARK();
	return 1;
}



JSOBJ Dir_iterGetValue(JSOBJ obj, JSTYPEINFO *ti)
{
	PRINTMARK();
	return (PyObject *) ti->prv[PRV_ITER_ITEM_VALUE];
}

char *Dir_iterGetName(JSOBJ obj, JSTYPEINFO *ti, size_t *outLen)
{
	PyObject *curr = (PyObject *) ti->prv[PRV_ITER_ITEM_NAME];
	PRINTMARK();
	*outLen = PyString_GET_SIZE(curr);
	return PyString_AS_STRING(curr);
}




//=============================================================================
// List iteration functions 
// itemValue is borrowed from object (which is list). No refcounting
//=============================================================================
void List_iterBegin(JSOBJ obj, JSTYPEINFO *ti)
{
	ti->prv[PRV_ITER_INDEX] = (void *) 0;
	ti->prv[PRV_ITER_SIZE] = (void *) PyList_GET_SIZE( (PyObject *) obj);
	ti->prv[PRV_ITER_ITEM_VALUE] = NULL;
}

int List_iterNext(JSOBJ obj, JSTYPEINFO *ti)
{
	size_t i = (size_t) ti->prv[PRV_ITER_INDEX];
	size_t sz = (size_t) ti->prv[PRV_ITER_SIZE];
	PyObject *item;

	if (i >= sz)
	{
		return 0;
	}

	item = PyList_GET_ITEM (obj, i);

	ti->prv[PRV_ITER_ITEM_VALUE] = item;
	i ++;
	ti->prv[PRV_ITER_INDEX] = (void *) i;
	return 1;
}

void List_iterEnd(JSOBJ obj, JSTYPEINFO *ti)
{
}

JSOBJ List_iterGetValue(JSOBJ obj, JSTYPEINFO *ti)
{
	return (PyObject *) ti->prv[PRV_ITER_ITEM_VALUE];
}

char *List_iterGetName(JSOBJ obj, JSTYPEINFO *ti, size_t *outLen)
{
	return NULL;
}

//=============================================================================
// Dict iteration functions 
// itemName might converted to string (Python_Str). Do refCounting
// itemValue is borrowed from object (which is dict). No refCounting
//=============================================================================
void Dict_iterBegin(JSOBJ obj, JSTYPEINFO *ti)
{
	ti->prv[PRV_ITER_INDEX] = (void *) 0;
	ti->prv[PRV_ITER_ITEM_NAME] = NULL;
	ti->prv[PRV_ITER_ITEM_VALUE] = NULL;
	PRINTMARK();
}

int Dict_iterNext(JSOBJ _obj, JSTYPEINFO *ti)
{
	size_t i = (size_t) ti->prv[PRV_ITER_INDEX];
	PyObject *obj = (PyObject *) ti->prv[PRV_ITER_DICTOBJ];
	PyObject *itemName = (PyObject *) ti->prv[PRV_ITER_ITEM_NAME];
	PyObject *itemValue;

	//fprintf (stderr, "%s: ti=%p obj=%p, i=%u, sz=%u\n", __FUNCTION__, ti, _obj, i, sz);
	if (itemName)
	{
		Py_DECREF(itemName);
		ti->prv[PRV_ITER_ITEM_NAME] = itemName = NULL;
	}


	if (!PyDict_Next (obj, &((size_t) ti->prv[PRV_ITER_INDEX]), &itemName, &itemValue))
	{
		return 0;
	}
	if (!PyString_Check(itemName))
		itemName = PyObject_Str(itemName);
	else
		Py_INCREF(itemName);

	ti->prv[PRV_ITER_ITEM_NAME] = itemName;
	ti->prv[PRV_ITER_ITEM_VALUE] = itemValue;

	PRINTMARK();
	return 1;
}

void Dict_iterEnd(JSOBJ obj, JSTYPEINFO *ti)
{
	PyObject *itemName = (PyObject *) ti->prv[PRV_ITER_ITEM_NAME];

	if (itemName)
	{
		Py_DECREF(itemName);
		ti->prv[PRV_ITER_ITEM_NAME] = itemName = NULL;
	}
	Py_DECREF( (PyObject *) ti->prv[PRV_ITER_DICTOBJ]);
	PRINTMARK();
}

JSOBJ Dict_iterGetValue(JSOBJ obj, JSTYPEINFO *ti)
{
	return (PyObject *) ti->prv[PRV_ITER_ITEM_VALUE];
}

char *Dict_iterGetName(JSOBJ obj, JSTYPEINFO *ti, size_t *outLen)
{
	PyObject *curr = (PyObject *) ti->prv[PRV_ITER_ITEM_NAME];
	*outLen = PyString_GET_SIZE(curr);
	return PyString_AS_STRING(curr);
}


static void Object_getType(JSOBJ _obj, JSTYPEINFO *ti)
{
	PyObject *obj = (PyObject *) _obj;
	PyObject *toDictFunc;
	
	if (PyIter_Check(obj))
	{
		goto ISITERABLE;
	}

	if (PyInt_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = (void *) PyIntToINTEGER; ti->type = JT_INTEGER;
		return;
	}
	else 
	if (PyLong_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = (void *) PyLongToINTEGER; ti->type = JT_INTEGER;
		return;
	}
	else
	if (PyString_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = (void *) PyStringToUTF8; ti->type = JT_UTF8;
		return;
	}
	else
	if (PyUnicode_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = (void *) PyUnicodeToUTF8; ti->type = JT_UTF8;
		return;
	}
	else
	if (PyBool_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = NULL; 
		ti->type = (obj == Py_True) ? JT_TRUE : JT_FALSE;
		return;
	}
	else
	if (PyFloat_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = (void *) PyFloatToDOUBLE; ti->type = JT_DOUBLE;
		return;
	}
	else 
	if (PyDateTime_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = (void *) PyDateTimeToINTEGER; ti->type = JT_INTEGER;
		return;
	}
	else 
	if (PyDate_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = (void *) PyDateToINTEGER; ti->type = JT_INTEGER;
		return;
	}
	else
	if (obj == Py_None)
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_NULL;
		return;
	}


ISITERABLE:

	if (PyDict_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_OBJECT;
		ti->prv[PRV_ITER_BEGIN_FUNC] = Dict_iterBegin;
		ti->prv[PRV_ITER_END_FUNC] = Dict_iterEnd;
		ti->prv[PRV_ITER_NEXT_FUNC] = Dict_iterNext;
		ti->prv[PRV_ITER_GETVALUE_FUNC] = Dict_iterGetValue;
		ti->prv[PRV_ITER_GETNAME_FUNC] = Dict_iterGetName;
		ti->prv[PRV_ITER_DICTOBJ] = obj;
		Py_INCREF(obj);

		return;
	}
	else
	if (PyList_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_ARRAY;
		ti->prv[PRV_ITER_BEGIN_FUNC] = List_iterBegin;
		ti->prv[PRV_ITER_END_FUNC] = List_iterEnd;
		ti->prv[PRV_ITER_NEXT_FUNC] = List_iterNext;
		ti->prv[PRV_ITER_GETVALUE_FUNC] = List_iterGetValue;
		ti->prv[PRV_ITER_GETNAME_FUNC] = List_iterGetName;
		return;
	}
	else
	if (PyTuple_Check(obj))
	{
		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_ARRAY;
		ti->prv[PRV_ITER_BEGIN_FUNC] = Tuple_iterBegin;
		ti->prv[PRV_ITER_END_FUNC] = Tuple_iterEnd;
		ti->prv[PRV_ITER_NEXT_FUNC] = Tuple_iterNext;
		ti->prv[PRV_ITER_GETVALUE_FUNC] = Tuple_iterGetValue;
		ti->prv[PRV_ITER_GETNAME_FUNC] = Tuple_iterGetName;
		return;
	}


	toDictFunc = PyObject_GetAttrString(obj, "toDict");

	if (toDictFunc)
	{
		PyObject* tuple = PyTuple_New(0);
		PyObject* toDictResult = PyObject_Call(toDictFunc, tuple, NULL);
		Py_DECREF(tuple);
		Py_DECREF(toDictFunc);

		if (toDictResult == NULL)
		{
			PyErr_Clear();
			ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_NULL;
			return;
		}

		if (!PyDict_Check(toDictResult))
		{
			Py_DECREF(toDictResult);
			ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_NULL;
			return;
		}

		PRINTMARK();
		ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_OBJECT;
		ti->prv[PRV_ITER_BEGIN_FUNC] = Dict_iterBegin;
		ti->prv[PRV_ITER_END_FUNC] = Dict_iterEnd;
		ti->prv[PRV_ITER_NEXT_FUNC] = Dict_iterNext;
		ti->prv[PRV_ITER_GETVALUE_FUNC] = Dict_iterGetValue;
		ti->prv[PRV_ITER_GETNAME_FUNC] = Dict_iterGetName;
		ti->prv[PRV_ITER_DICTOBJ] = toDictResult;
		return;
	}

	PyErr_Clear();

	ti->prv[PRV_CONV_FUNC] = NULL; ti->type = JT_OBJECT;
	ti->prv[PRV_ITER_BEGIN_FUNC] = Dir_iterBegin;
	ti->prv[PRV_ITER_END_FUNC] = Dir_iterEnd;
	ti->prv[PRV_ITER_NEXT_FUNC] = Dir_iterNext;
	ti->prv[PRV_ITER_GETVALUE_FUNC] = Dir_iterGetValue;
	ti->prv[PRV_ITER_GETNAME_FUNC] = Dir_iterGetName;

	return;
}


static void *Object_getValue(JSOBJ obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen)
{
	void *ret;
	PFN_PyTypeToJSON pfnFunc = (PFN_PyTypeToJSON) ti->prv[PRV_CONV_FUNC];
	PRINTMARK();
	ret = pfnFunc (obj, ti, outValue, _outLen);
	PRINTMARK();
	return ret;
}

void Object_releaseObject(JSOBJ *_obj)
{
	PyObject *obj = (PyObject *) _obj;
	Py_DECREF(obj);
}

void Object_releaseValue(JSTYPEINFO *ti)
{
	PyObject *obj = (PyObject *) ti->prv[PRV_CONV_NEWOBJ];
	//fprintf (stderr, "%s: Object %p was released!\n", __FUNCTION__, obj);
	Py_DECREF(obj);
}

void Object_iterBegin(JSOBJ obj, JSTYPEINFO *ti)
{
	JSPFN_ITERBEGIN pfnFunc = (JSPFN_ITERBEGIN) ti->prv[PRV_ITER_BEGIN_FUNC];
	pfnFunc (obj, ti);
}

int Object_iterNext(JSOBJ obj, JSTYPEINFO *ti)
{
	JSPFN_ITERNEXT pfnFunc = (JSPFN_ITERNEXT) ti->prv[PRV_ITER_NEXT_FUNC];
	return pfnFunc (obj, ti);
}

void Object_iterEnd(JSOBJ obj, JSTYPEINFO *ti)
{
	JSPFN_ITEREND pfnFunc = (JSPFN_ITEREND) ti->prv[PRV_ITER_END_FUNC];
	pfnFunc (obj, ti);
}

JSOBJ Object_iterGetValue(JSOBJ obj, JSTYPEINFO *ti)
{
	JSPFN_ITERGETVALUE pfnFunc = (JSPFN_ITERGETVALUE) ti->prv[PRV_ITER_GETVALUE_FUNC];
	return pfnFunc (obj, ti);
}

char *Object_iterGetName(JSOBJ obj, JSTYPEINFO *ti, size_t *outLen)
{
	JSPFN_ITERGETNAME pfnFunc = (JSPFN_ITERGETNAME) ti->prv[PRV_ITER_GETNAME_FUNC];
	return pfnFunc (obj, ti, outLen);
}


static JSONObjectEncoder g_encState = 
{
	Object_getType, //void (*getType)(JSOBJ obj, JSTYPEINFO *ti);
	Object_getValue, //void *(*getValue)(JSOBJ obj, JSTYPEINFO *ti, void *outValue, size_t *_outLen);
	Object_iterBegin, //JSPFN_ITERBEGIN iterBegin;
	Object_iterNext, //JSPFN_ITERNEXT iterNext;
	Object_iterEnd, //JSPFN_ITEREND iterEnd;
	Object_iterGetValue, //JSPFN_ITERGETVALUE iterGetValue;
	Object_iterGetName, //JSPFN_ITERGETNAME iterGetName;
	Object_releaseValue, //void (*releaseValue)(JSTYPEINFO *ti);
	PyObject_Malloc, //JSPFN_MALLOC malloc;
	PyObject_Realloc, //JSPFN_REALLOC realloc;
	PyObject_Free//JSPFN_FREE free;
};


PyObject* objToJSON(PyObject* self, PyObject *arg)
{
	char buffer[65536];
	char *ret;
	PyObject *newobj;

	//FIXME: Add support for max recursion to eliminiate OOM scenario on cyclic references
	ret = JSON_EncodeObject (arg, &g_encState, buffer, sizeof (buffer));
	newobj = PyString_FromString (ret);

	if (ret != buffer)
	{
		g_encState.free (ret);
	}

	return newobj;
}
