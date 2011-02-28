#include <Python.h>

/* objToJSON */
PyObject* objToJSON(PyObject* self, PyObject *arg);
void initObjToJSON();


static PyMethodDef ujsonMethods[] = {
	{"encode", objToJSON, METH_O, "Converts arbitrary object recursivly into JSON as string"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};



PyMODINIT_FUNC
initujson(void)
{
	initObjToJSON();
    Py_InitModule("ujson", ujsonMethods);

}