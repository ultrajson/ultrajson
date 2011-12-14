#include <Python.h>
#include <ultrajson.h>




void Object_objectAddKey(JSOBJ obj, JSOBJ name, JSOBJ value)
{
	PyDict_SetItem (obj, name, value);
	Py_DECREF( (PyObject *) name);
	Py_DECREF( (PyObject *) value);
	return;
}

void Object_arrayAddItem(JSOBJ obj, JSOBJ value)
{
	PyList_Append(obj, value);
	Py_DECREF( (PyObject *) value);
	return;
}

JSOBJ Object_newString(wchar_t *start, wchar_t *end)
{
	return PyUnicode_FromWideChar (start, (end - start));
}

JSOBJ Object_newTrue(void)
{ 
	Py_RETURN_TRUE;
}

JSOBJ Object_newFalse(void)
{
	Py_RETURN_FALSE;
}

JSOBJ Object_newNull(void)
{
	Py_RETURN_NONE;
}

JSOBJ Object_newObject(void)
{
	return PyDict_New();
}

JSOBJ Object_newArray(void)
{
	return PyList_New(0);
}

JSOBJ Object_newInteger(JSINT32 value)
{
	return PyInt_FromLong( (long) value);
}

JSOBJ Object_newLong(JSINT64 value)
{
	return PyLong_FromLongLong (value);
}

JSOBJ Object_newDouble(double value)
{ 
	return PyFloat_FromDouble(value);
}

static void Object_releaseObject(JSOBJ obj)
{
	Py_DECREF( ((PyObject *)obj));
}



PyObject* JSONToObj(PyObject* self, PyObject *arg)
{
	PyObject *ret;
	PyObject *sarg;
	JSONObjectDecoder decoder = 
	{
		Object_newString,
		Object_objectAddKey,
		Object_arrayAddItem,
		Object_newTrue,
		Object_newFalse,
		Object_newNull,
		Object_newObject,
		Object_newArray,
		Object_newInteger,
		Object_newLong,
		Object_newDouble,
		Object_releaseObject,
		PyObject_Malloc,
		PyObject_Free,
		PyObject_Realloc
	};

	if (PyString_Check(arg))
	{
		sarg = arg;
	}
	else
	if (PyUnicode_Check(arg))
	{
		sarg = PyUnicode_AsUTF8String(arg);
		if (sarg == NULL)
		{
			//Exception raised above us by codec according to docs
			return NULL;
		}
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "Expected String or Unicode");
		return NULL;
	}

	decoder.errorStr = NULL;
	decoder.errorOffset = NULL;
	
	ret = JSON_DecodeObject(&decoder, PyString_AS_STRING(sarg), PyString_GET_SIZE(sarg)); 

	if (sarg != arg)
	{
		Py_DECREF(sarg);
	}

	if (decoder.errorStr)
	{
		/*
		FIXME: It's possible to give a much nicer error message here with actual failing element in input etc*/
		
		PyErr_Format (PyExc_ValueError, "%s", decoder.errorStr);
		
		if (ret)
		{
			Py_DECREF( (PyObject *) ret);
		}
		
		return NULL;
	}
	
	return ret;
}

PyObject* JSONFileToObj(PyObject* self, PyObject *file)
{
	PyObject *read;
	PyObject *string;
	PyObject *result;

	if (!PyObject_HasAttrString (file, "read"))
	{
		PyErr_Format (PyExc_TypeError, "expected file");
		return NULL;
	}

	read = PyObject_GetAttrString (file, "read");

	if (!PyCallable_Check (read)) {
		Py_XDECREF(read);
		PyErr_Format (PyExc_TypeError, "expected file");
		return NULL;
	}

	string = PyObject_CallObject (read, NULL);
	Py_XDECREF(read);

	if (string == NULL)
	{
		return NULL;
	}

	result = JSONToObj (self, string);
	Py_XDECREF(string);

	if (result == NULL) {
		return NULL;
	}

	return result;
}

