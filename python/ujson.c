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
#include "version.h"

/* objToJSON */
PyObject* objToJSON(PyObject* self, PyObject *args, PyObject *kwargs);
void initObjToJSON(void);

/* JSONToObj */
PyObject* JSONToObj(PyObject* self, PyObject *arg);

/* objToJSONFile */
PyObject* objToJSONFile(PyObject* self, PyObject *args, PyObject *kwargs);

/* JSONFileToObj */
PyObject* JSONFileToObj(PyObject* self, PyObject *file);


static PyMethodDef ujsonMethods[] = {
    {"encode", (PyCFunction) objToJSON, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursivly into JSON. Use ensure_ascii=false to output UTF-8. Pass in double_precision to alter the maximum digit precision with doubles"},
    {"decode", (PyCFunction) JSONToObj, METH_O, "Converts JSON as string to dict object structure"},
    {"dumps", (PyCFunction) objToJSON, METH_VARARGS | METH_KEYWORDS,  "Converts arbitrary object recursivly into JSON. Use ensure_ascii=false to output UTF-8"},
    {"loads", (PyCFunction) JSONToObj, METH_O,  "Converts JSON as string to dict object structure"},
    {"dump", (PyCFunction) objToJSONFile, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON file. Use ensure_ascii=false to output UTF-8"},
    {"load", (PyCFunction) JSONFileToObj, METH_O, "Converts JSON as file to dict object structure"},
    {NULL, NULL, 0, NULL}       /* Sentinel */
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "ujson",
    0,              /* m_doc */
    -1,             /* m_size */
    ujsonMethods,   /* m_methods */
    NULL,           /* m_reload */
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL            /* m_free */
};

#define PYMODINITFUNC       PyObject *PyInit_ujson(void)
#define PYMODULE_CREATE()   PyModule_Create(&moduledef)
#define MODINITERROR        return NULL

#else

#define PYMODINITFUNC       PyMODINIT_FUNC initujson(void)
#define PYMODULE_CREATE()   Py_InitModule("ujson", ujsonMethods)
#define MODINITERROR        return

#endif

PYMODINITFUNC
{
    PyObject *module;
    PyObject *version_string;

    initObjToJSON();   
    module = PYMODULE_CREATE();

    if (module == NULL)
    {
        MODINITERROR;
    }

    version_string = PyString_FromString (UJSON_VERSION);
    PyModule_AddObject (module, "__version__", version_string);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
