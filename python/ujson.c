#include <Python.h>
#include "version.h"

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
	{"encode", objToJSON, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursivly into JSON. Use ensure_ascii=false to output UTF-8. Pass in double_precision to alter the maximum digit precision with doubles"},
	{"decode", JSONToObj, METH_O, "Converts JSON as string to dict object structure"},
	{"dumps", objToJSON, METH_VARARGS | METH_KEYWORDS,  "Converts arbitrary object recursivly into JSON. Use ensure_ascii=false to output UTF-8"},
	{"loads", JSONToObj, METH_O,  "Converts JSON as string to dict object structure"},
	{"dump", objToJSONFile, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON file. Use ensure_ascii=false to output UTF-8"},
	{"load", JSONFileToObj, METH_O, "Converts JSON as file to dict object structure"},
	{NULL, NULL, 0, NULL}		/* Sentinel */
};



PyMODINIT_FUNC
initujson(void)
{
	PyObject *module;
	PyObject *version_string;

	initObjToJSON();
	module = Py_InitModule("ujson", ujsonMethods);

	version_string = PyString_FromString (UJSON_VERSION);
	PyModule_AddObject (module, "__version__", version_string);
}
