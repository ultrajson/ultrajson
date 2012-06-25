/*
Copyright (c) 2011-2012, ESN Social Software AB and Jonas Tarnstrom
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the ESN Social Software AB nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ESN SOCIAL SOFTWARE AB OR JONAS TARNSTROM BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
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

#include "py_defines.h"
#include <stdio.h>
#include <datetime.h>
#include <ultrajson.h>

#define EPOCH_ORD 719163

typedef void *(*PFN_PyTypeToJSON)(JSOBJ obj, JSONTypeContext *ti, void *outValue, size_t *_outLen);


#if (PY_VERSION_HEX < 0x02050000)
typedef ssize_t Py_ssize_t;
#endif


typedef struct __TypeContext
{
    JSPFN_ITERBEGIN iterBegin;
    JSPFN_ITEREND iterEnd;
    JSPFN_ITERNEXT iterNext;
    JSPFN_ITERGETNAME iterGetName;
    JSPFN_ITERGETVALUE iterGetValue;
    PFN_PyTypeToJSON PyTypeToJSON;
    PyObject *newObj;
    PyObject *dictObj;
    Py_ssize_t index;
    Py_ssize_t size;
    PyObject *itemValue;
    PyObject *itemName;
    PyObject *attrList;

    JSINT64 longValue;

} TypeContext;

#define GET_TC(__ptrtc) ((TypeContext *)((__ptrtc)->prv))

struct PyDictIterState
{
    PyObject *keys;
    size_t i;
    size_t sz;
};


//#define PRINTMARK() fprintf(stderr, "%s: MARK(%d)\n", __FILE__, __LINE__)     
#define PRINTMARK()         

void initObjToJSON(void)
{
    PyDateTime_IMPORT;
}

static void *PyIntToINT32(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    PyObject *obj = (PyObject *) _obj;
    *((JSINT32 *) outValue) = PyInt_AS_LONG (obj);
    return NULL;
}

static void *PyIntToINT64(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    PyObject *obj = (PyObject *) _obj;
    *((JSINT64 *) outValue) = PyInt_AS_LONG (obj);
    return NULL;
}

static void *PyLongToINT64(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    *((JSINT64 *) outValue) = GET_TC(tc)->longValue;
    return NULL;
}

static void *PyFloatToDOUBLE(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    PyObject *obj = (PyObject *) _obj;
    *((double *) outValue) = PyFloat_AS_DOUBLE (obj);
    return NULL;
}

static void *PyStringToUTF8(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    PyObject *obj = (PyObject *) _obj;
    *_outLen = PyString_GET_SIZE(obj);
    return PyString_AS_STRING(obj);
}

static void *PyUnicodeToUTF8(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    PyObject *obj = (PyObject *) _obj;
    PyObject *newObj = PyUnicode_EncodeUTF8 (PyUnicode_AS_UNICODE(obj), PyUnicode_GET_SIZE(obj), NULL);

    GET_TC(tc)->newObj = newObj;

    *_outLen = PyString_GET_SIZE(newObj);
    return PyString_AS_STRING(newObj);
}

static void *PyDateTimeToINT64(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    PyObject *obj = (PyObject *) _obj;
    PyObject *date, *ord;
    int y, m, d, h, mn, s, days;

    y = PyDateTime_GET_YEAR(obj);
    m = PyDateTime_GET_MONTH(obj);
    d = PyDateTime_GET_DAY(obj);
    h = PyDateTime_DATE_GET_HOUR(obj);
    mn = PyDateTime_DATE_GET_MINUTE(obj);
    s = PyDateTime_DATE_GET_SECOND(obj);

    date = PyDate_FromDate(y, m, 1);
    ord = PyObject_CallMethod(date, "toordinal", NULL);
    days = PyInt_AS_LONG(ord) - EPOCH_ORD + d - 1;
    Py_DECREF(date);
    Py_DECREF(ord);
    *( (JSINT64 *) outValue) = (((JSINT64) ((days * 24 + h) * 60 + mn)) * 60 + s);
    return NULL;
}

static void *PyDateToINT64(JSOBJ _obj, JSONTypeContext *tc, void *outValue, size_t *_outLen)
{
    PyObject *obj = (PyObject *) _obj;
    PyObject *date, *ord;
    int y, m, d, days;

    y = PyDateTime_GET_YEAR(obj);
    m = PyDateTime_GET_MONTH(obj);
    d = PyDateTime_GET_DAY(obj);

    date = PyDate_FromDate(y, m, 1);
    ord = PyObject_CallMethod(date, "toordinal", NULL);
    days = PyInt_AS_LONG(ord) - EPOCH_ORD + d - 1;
    Py_DECREF(date);
    Py_DECREF(ord);
    *( (JSINT64 *) outValue) = ((JSINT64) days * 86400);

    return NULL;
}

//=============================================================================
// Tuple iteration functions 
// itemValue is borrowed reference, no ref counting
//=============================================================================
void Tuple_iterBegin(JSOBJ obj, JSONTypeContext *tc)
{
    GET_TC(tc)->index = 0;
    GET_TC(tc)->size = PyTuple_GET_SIZE( (PyObject *) obj);
    GET_TC(tc)->itemValue = NULL;
}

int Tuple_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
    PyObject *item;

    if (GET_TC(tc)->index >= GET_TC(tc)->size)
    {
        return 0;
    }

    item = PyTuple_GET_ITEM (obj, GET_TC(tc)->index);

    GET_TC(tc)->itemValue = item;
    GET_TC(tc)->index ++;
    return 1;
}

void Tuple_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
}

JSOBJ Tuple_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
    return GET_TC(tc)->itemValue;
}

char *Tuple_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
    return NULL;
}

//=============================================================================
// Dir iteration functions 
// itemName ref is borrowed from PyObject_Dir (attrList). No refcount
// itemValue ref is from PyObject_GetAttr. Ref counted
//=============================================================================
void Dir_iterBegin(JSOBJ obj, JSONTypeContext *tc)
{
    GET_TC(tc)->attrList = PyObject_Dir(obj); 
    GET_TC(tc)->index = 0;
    GET_TC(tc)->size = PyList_GET_SIZE(GET_TC(tc)->attrList);
    PRINTMARK();
}

void Dir_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
    if (GET_TC(tc)->itemValue)
    {
        Py_DECREF(GET_TC(tc)->itemValue);
        GET_TC(tc)->itemValue = NULL;
    }

    if (GET_TC(tc)->itemName)
    {
        Py_DECREF(GET_TC(tc)->itemName);
        GET_TC(tc)->itemName = NULL;
    }

    Py_DECREF( (PyObject *) GET_TC(tc)->attrList);
    PRINTMARK();
}

int Dir_iterNext(JSOBJ _obj, JSONTypeContext *tc)
{
    PyObject *obj = (PyObject *) _obj;
    PyObject *itemValue = GET_TC(tc)->itemValue;
    PyObject *itemName = GET_TC(tc)->itemName;
    PyObject* attr;
    PyObject* attrName;
    char* attrStr;


    if (itemValue)
    {
        Py_DECREF(GET_TC(tc)->itemValue);
        GET_TC(tc)->itemValue = itemValue = NULL;
    }

    if (itemName)
    {
        Py_DECREF(GET_TC(tc)->itemName);
        GET_TC(tc)->itemName = itemName = NULL;
    }

    for (; GET_TC(tc)->index  < GET_TC(tc)->size; GET_TC(tc)->index ++)
    {
        attrName = PyList_GET_ITEM(GET_TC(tc)->attrList, GET_TC(tc)->index);
#if PY_MAJOR_VERSION >= 3
        attr = PyUnicode_AsUTF8String(attrName);
#else 
        attr = attrName;
        Py_INCREF(attr);
#endif
        attrStr = PyString_AS_STRING(attr);

        if (attrStr[0] == '_')
        {
            PRINTMARK();
            Py_DECREF(attr);
            continue;
        }

        itemValue = PyObject_GetAttr(obj, attrName);
        if (itemValue == NULL)
        {
            PyErr_Clear();
            Py_DECREF(attr);
            PRINTMARK();
            continue;
        }

        if (PyCallable_Check(itemValue))
        {
            Py_DECREF(itemValue);
            Py_DECREF(attr);
            PRINTMARK();
            continue;
        }

        PRINTMARK();
        itemName = attr;
        break;
    }

    if (itemName == NULL)
    {
        GET_TC(tc)->index = GET_TC(tc)->size;
        GET_TC(tc)->itemValue = NULL;
        return 0;
    }

    GET_TC(tc)->itemName = itemName;
    GET_TC(tc)->itemValue = itemValue;
    GET_TC(tc)->index ++;
    
    PRINTMARK();
    return 1;
}



JSOBJ Dir_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
    PRINTMARK();
    return GET_TC(tc)->itemValue;
}

char *Dir_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
    PRINTMARK();
    *outLen = PyString_GET_SIZE(GET_TC(tc)->itemName);
    return PyString_AS_STRING(GET_TC(tc)->itemName);
}




//=============================================================================
// List iteration functions 
// itemValue is borrowed from object (which is list). No refcounting
//=============================================================================
void List_iterBegin(JSOBJ obj, JSONTypeContext *tc)
{
    GET_TC(tc)->index =  0;
    GET_TC(tc)->size = PyList_GET_SIZE( (PyObject *) obj);
}

int List_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
    if (GET_TC(tc)->index >= GET_TC(tc)->size)
    {
        PRINTMARK();
        return 0;
    }

    GET_TC(tc)->itemValue = PyList_GET_ITEM (obj, GET_TC(tc)->index);
    GET_TC(tc)->index ++;
    return 1;
}

void List_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
}

JSOBJ List_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
    return GET_TC(tc)->itemValue;
}

char *List_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
    return NULL;
}

//=============================================================================
// Dict iteration functions 
// itemName might converted to string (Python_Str). Do refCounting
// itemValue is borrowed from object (which is dict). No refCounting
//=============================================================================
void Dict_iterBegin(JSOBJ obj, JSONTypeContext *tc)
{
    GET_TC(tc)->index = 0;
    PRINTMARK();
}

int Dict_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
#if PY_MAJOR_VERSION >= 3
    PyObject* itemNameTmp;
#endif

    if (GET_TC(tc)->itemName)
    {
        Py_DECREF(GET_TC(tc)->itemName);
        GET_TC(tc)->itemName = NULL;
    }


    if (!PyDict_Next ( (PyObject *)GET_TC(tc)->dictObj, &GET_TC(tc)->index, &GET_TC(tc)->itemName, &GET_TC(tc)->itemValue))
    {
        PRINTMARK();
        return 0;
    }

    if (PyUnicode_Check(GET_TC(tc)->itemName))
    {
        GET_TC(tc)->itemName = PyUnicode_AsUTF8String (GET_TC(tc)->itemName);
    }
    else
    if (!PyString_Check(GET_TC(tc)->itemName))
    {
        GET_TC(tc)->itemName = PyObject_Str(GET_TC(tc)->itemName);
#if PY_MAJOR_VERSION >= 3
        itemNameTmp = GET_TC(tc)->itemName; 
        GET_TC(tc)->itemName = PyUnicode_AsUTF8String (GET_TC(tc)->itemName);
        Py_DECREF(itemNameTmp);
#endif
    }
    else 
    {
        Py_INCREF(GET_TC(tc)->itemName);
    }
    PRINTMARK();
    return 1;
}

void Dict_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
    if (GET_TC(tc)->itemName)
    {
        Py_DECREF(GET_TC(tc)->itemName);
        GET_TC(tc)->itemName = NULL;
    }
    Py_DECREF(GET_TC(tc)->dictObj);
    PRINTMARK();
}

JSOBJ Dict_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
    return GET_TC(tc)->itemValue;
}

char *Dict_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
    *outLen = PyString_GET_SIZE(GET_TC(tc)->itemName);
    return PyString_AS_STRING(GET_TC(tc)->itemName);
}


void Object_beginTypeContext (JSOBJ _obj, JSONTypeContext *tc)
{
    PyObject *obj, *exc, *toDictFunc;
    TypeContext *pc;
    PRINTMARK();
    if (!_obj) {
        tc->type = JT_INVALID;
        return;
    }

    obj = (PyObject*) _obj;

    tc->prv = PyObject_Malloc(sizeof(TypeContext));
    pc = (TypeContext *) tc->prv;
    if (!pc)
    {
        tc->type = JT_INVALID;
        PyErr_NoMemory();
        return;
    }
    pc->newObj = NULL;
    pc->dictObj = NULL;
    pc->itemValue = NULL;
    pc->itemName = NULL;
    pc->attrList = NULL;
    pc->index = 0;
    pc->size = 0;
    pc->longValue = 0;
    
    if (PyIter_Check(obj))
    {
        goto ISITERABLE;
    }

    if (PyBool_Check(obj))
    {
        PRINTMARK();
        tc->type = (obj == Py_True) ? JT_TRUE : JT_FALSE;
        return;
    }
    else
    if (PyLong_Check(obj))
    {
        PRINTMARK();
        pc->PyTypeToJSON = PyLongToINT64;
        tc->type = JT_LONG;
        GET_TC(tc)->longValue = PyLong_AsLongLong(obj);

        exc = PyErr_Occurred();

        if (exc && PyErr_ExceptionMatches(PyExc_OverflowError))
        {
            PRINTMARK();
            goto INVALID;
        }

        return;
    }
    else
    if (PyInt_Check(obj))
    {
        PRINTMARK();
#ifdef _LP64
        pc->PyTypeToJSON = PyIntToINT64; tc->type = JT_LONG;
#else
        pc->PyTypeToJSON = PyIntToINT32; tc->type = JT_INT;
#endif
        return;
    }
    else
    if (PyString_Check(obj))
    {
        PRINTMARK();
        pc->PyTypeToJSON = PyStringToUTF8; tc->type = JT_UTF8;
        return;
    }
    else
    if (PyUnicode_Check(obj))
    {
        PRINTMARK();
        pc->PyTypeToJSON = PyUnicodeToUTF8; tc->type = JT_UTF8;
        return;
    }
    else
    if (PyFloat_Check(obj))
    {
        PRINTMARK();
        pc->PyTypeToJSON = PyFloatToDOUBLE; tc->type = JT_DOUBLE;
        return;
    }
    else 
    if (PyDateTime_Check(obj))
    {
        PRINTMARK();
        pc->PyTypeToJSON = PyDateTimeToINT64; tc->type = JT_LONG;
        return;
    }
    else 
    if (PyDate_Check(obj))
    {
        PRINTMARK();
        pc->PyTypeToJSON = PyDateToINT64; tc->type = JT_LONG;
        return;
    }
    else
    if (obj == Py_None)
    {
        PRINTMARK();
        tc->type = JT_NULL;
        return;
    }


ISITERABLE:

    if (PyDict_Check(obj))
    {
        PRINTMARK();
        tc->type = JT_OBJECT;
        pc->iterBegin = Dict_iterBegin;
        pc->iterEnd = Dict_iterEnd;
        pc->iterNext = Dict_iterNext;
        pc->iterGetValue = Dict_iterGetValue;
        pc->iterGetName = Dict_iterGetName;
        pc->dictObj = obj;
        Py_INCREF(obj);

        return;
    }
    else
    if (PyList_Check(obj))
    {
        PRINTMARK();
        tc->type = JT_ARRAY;
        pc->iterBegin = List_iterBegin;
        pc->iterEnd = List_iterEnd;
        pc->iterNext = List_iterNext;
        pc->iterGetValue = List_iterGetValue;
        pc->iterGetName = List_iterGetName;
        return;
    }
    else
    if (PyTuple_Check(obj))
    {
        PRINTMARK();
        tc->type = JT_ARRAY;
        pc->iterBegin = Tuple_iterBegin;
        pc->iterEnd = Tuple_iterEnd;
        pc->iterNext = Tuple_iterNext;
        pc->iterGetValue = Tuple_iterGetValue;
        pc->iterGetName = Tuple_iterGetName;
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
            tc->type = JT_NULL;
            return;
        }

        if (!PyDict_Check(toDictResult))
        {
            Py_DECREF(toDictResult);
            tc->type = JT_NULL;
            return;
        }

        PRINTMARK();
        tc->type = JT_OBJECT;
        pc->iterBegin = Dict_iterBegin;
        pc->iterEnd = Dict_iterEnd;
        pc->iterNext = Dict_iterNext;
        pc->iterGetValue = Dict_iterGetValue;
        pc->iterGetName = Dict_iterGetName;
        pc->dictObj = toDictResult;
        return;
    }

    PyErr_Clear();

    tc->type = JT_OBJECT;
    pc->iterBegin = Dir_iterBegin;
    pc->iterEnd = Dir_iterEnd;
    pc->iterNext = Dir_iterNext;
    pc->iterGetValue = Dir_iterGetValue;
    pc->iterGetName = Dir_iterGetName;

    return;

INVALID:
    tc->type = JT_INVALID;
    PyObject_Free(tc->prv);
    tc->prv = NULL;
    return;
}


void Object_endTypeContext(JSOBJ obj, JSONTypeContext *tc)
{
    Py_XDECREF(GET_TC(tc)->newObj);

    PyObject_Free(tc->prv);
    tc->prv = NULL;
}

const char *Object_getStringValue(JSOBJ obj, JSONTypeContext *tc, size_t *_outLen)
{
    return GET_TC(tc)->PyTypeToJSON (obj, tc, NULL, _outLen);
}

JSINT64 Object_getLongValue(JSOBJ obj, JSONTypeContext *tc)
{
    JSINT64 ret;
    GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);

    return ret;
}

JSINT32 Object_getIntValue(JSOBJ obj, JSONTypeContext *tc)
{
    JSINT32 ret;
    GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
    return ret;
}


double Object_getDoubleValue(JSOBJ obj, JSONTypeContext *tc)
{
    double ret;
    GET_TC(tc)->PyTypeToJSON (obj, tc, &ret, NULL);
    return ret;
}

static void Object_releaseObject(JSOBJ _obj)
{
    Py_DECREF( (PyObject *) _obj);
}



void Object_iterBegin(JSOBJ obj, JSONTypeContext *tc)
{
    GET_TC(tc)->iterBegin(obj, tc);
}

int Object_iterNext(JSOBJ obj, JSONTypeContext *tc)
{
    return GET_TC(tc)->iterNext(obj, tc);
}

void Object_iterEnd(JSOBJ obj, JSONTypeContext *tc)
{
    GET_TC(tc)->iterEnd(obj, tc);
}

JSOBJ Object_iterGetValue(JSOBJ obj, JSONTypeContext *tc)
{
    return GET_TC(tc)->iterGetValue(obj, tc);
}

char *Object_iterGetName(JSOBJ obj, JSONTypeContext *tc, size_t *outLen)
{
    return GET_TC(tc)->iterGetName(obj, tc, outLen);
}


PyObject* objToJSON(PyObject* self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "obj", "ensure_ascii", "double_precision", NULL};

    char buffer[65536];
    char *ret;
    PyObject *newobj;
    PyObject *oinput = NULL;
    PyObject *oensureAscii = NULL;
    int idoublePrecision = 5; // default double precision setting

    JSONObjectEncoder encoder = 
    {
        Object_beginTypeContext,    //void (*beginTypeContext)(JSOBJ obj, JSONTypeContext *tc);
        Object_endTypeContext, //void (*endTypeContext)(JSOBJ obj, JSONTypeContext *tc);
        Object_getStringValue, //const char *(*getStringValue)(JSOBJ obj, JSONTypeContext *tc, size_t *_outLen);
        Object_getLongValue, //JSLONG (*getLongValue)(JSOBJ obj, JSONTypeContext *tc);
        Object_getIntValue, //JSLONG (*getLongValue)(JSOBJ obj, JSONTypeContext *tc);
        Object_getDoubleValue, //double (*getDoubleValue)(JSOBJ obj, JSONTypeContext *tc);
        Object_iterBegin, //JSPFN_ITERBEGIN iterBegin;
        Object_iterNext, //JSPFN_ITERNEXT iterNext;
        Object_iterEnd, //JSPFN_ITEREND iterEnd;
        Object_iterGetValue, //JSPFN_ITERGETVALUE iterGetValue;
        Object_iterGetName, //JSPFN_ITERGETNAME iterGetName;
        Object_releaseObject, //void (*releaseValue)(JSONTypeContext *ti);
        PyObject_Malloc, //JSPFN_MALLOC malloc;
        PyObject_Realloc, //JSPFN_REALLOC realloc;
        PyObject_Free, //JSPFN_FREE free;
        -1, //recursionMax
        idoublePrecision,
        1, //forceAscii
    };


    PRINTMARK();

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Oi", kwlist, &oinput, &oensureAscii, &idoublePrecision))
    {
        return NULL;
    }

    
    if (oensureAscii != NULL && !PyObject_IsTrue(oensureAscii))
    {
        encoder.forceASCII = 0;
    }

    encoder.doublePrecision = idoublePrecision;

    PRINTMARK();
    ret = JSON_EncodeObject (oinput, &encoder, buffer, sizeof (buffer));
    PRINTMARK();

    if (PyErr_Occurred())
    {
        return NULL;
    }

    if (encoder.errorMsg)
    {
        if (ret != buffer)
        {
            encoder.free (ret);
        }

        PyErr_Format (PyExc_OverflowError, "%s", encoder.errorMsg);
        return NULL;
    }

    newobj = PyString_FromString (ret);

    if (ret != buffer)
    {
        encoder.free (ret);
    }

    PRINTMARK();

    return newobj;
}

PyObject* objToJSONFile(PyObject* self, PyObject *args, PyObject *kwargs)
{
    PyObject *data;
    PyObject *file;
    PyObject *string;
    PyObject *write;
    PyObject *argtuple;

    PRINTMARK();

    if (!PyArg_ParseTuple (args, "OO", &data, &file)) {
        return NULL;
    }

    if (!PyObject_HasAttrString (file, "write"))
    {
        PyErr_Format (PyExc_TypeError, "expected file");
        return NULL;
    }

    write = PyObject_GetAttrString (file, "write");

    if (!PyCallable_Check (write)) {
        Py_XDECREF(write);
        PyErr_Format (PyExc_TypeError, "expected file");
        return NULL;
    }

    argtuple = PyTuple_Pack(1, data);

    string = objToJSON (self, argtuple, kwargs);

    if (string == NULL)
    {
        Py_XDECREF(write);
        Py_XDECREF(argtuple);
        return NULL;
    }

    Py_XDECREF(argtuple);

    argtuple = PyTuple_Pack (1, string);
    if (argtuple == NULL)
    {
        Py_XDECREF(write);
        return NULL;
    }
    if (PyObject_CallObject (write, argtuple) == NULL)
    {
        Py_XDECREF(write);
        Py_XDECREF(argtuple);
        return NULL;
    }

    Py_XDECREF(write);
    Py_DECREF(argtuple);
    Py_XDECREF(string);

    PRINTMARK();

    Py_RETURN_NONE;
    

}

