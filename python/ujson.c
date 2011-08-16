#include <Python.h>

/* objToJSON */
PyObject* objToJSON(PyObject* self, PyObject *arg);
void initObjToJSON();

/* JSONToObj */
PyObject* JSONToObj(PyObject* self, PyObject *arg);

/* objToJSONFile */
PyObject* objToJSONFile(PyObject* self, PyObject *args);

/* JSONFileToObj */
PyObject* JSONFileToObj(PyObject* self, PyObject *file);


static PyMethodDef ujsonMethods[] = {
	{"encode", objToJSON, METH_O, "Converts arbitrary object recursivly into JSON"},
	{"decode", JSONToObj, METH_O, "Converts JSON as string to dict object structure"},
	{"dumps", objToJSON, METH_O,  "Converts arbitrary object recursivly into JSON"},
	{"loads", JSONToObj, METH_O,  "Converts JSON as string to dict object structure"},
	{"dump", objToJSONFile, METH_VARARGS, "Converts arbitrary object recursively into JSON file"},
	{"load", JSONFileToObj, METH_O, "Converts JSON as file to dict object structure"},
	{NULL, NULL, 0, NULL}		/* Sentinel */
};



PyMODINIT_FUNC
initujson(void)
{
	initObjToJSON();
	Py_InitModule("ujson", ujsonMethods);

}
