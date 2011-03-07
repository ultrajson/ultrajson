#include <Python.h>

/* objToJSON */
PyObject* objToJSON(PyObject* self, PyObject *arg);
void initObjToJSON();

/* JSONToObj */
PyObject* JSONToObj(PyObject* self, PyObject *arg);


static PyMethodDef ujsonMethods[] = {
	{"encode", objToJSON, METH_O, "Converts arbitrary object recursivly into JSON"},
	{"decode", JSONToObj, METH_O, "Converts JSON as string to dict object structure"},
	{"dumps", objToJSON, METH_O,  "Converts arbitrary object recursivly into JSON"},
	{"loads", JSONToObj, METH_O,  "Converts JSON as string to dict object structure"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};



PyMODINIT_FUNC
initujson(void)
{
	initObjToJSON();
    Py_InitModule("ujson", ujsonMethods);

}