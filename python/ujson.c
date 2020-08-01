/*
Developed by ESN, an Electronic Arts Inc. studio.
Copyright (c) 2014, Electronic Arts Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of ESN, Electronic Arts Inc. nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS INC. BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Portions of code from MODP_ASCII - Ascii transformations (upper/lower, etc)
http://code.google.com/p/stringencoders/
Copyright (c) 2007  Nick Galbreath -- nickg [at] modp [dot] com. All rights reserved.

Numeric decoder derived from from TCL library
http://www.opensource.apple.com/source/tcl/tcl-14/tcl/license.terms
* Copyright (c) 1988-1993 The Regents of the University of California.
* Copyright (c) 1994 Sun Microsystems, Inc.
*/

#include <Python.h>
#include "version.h"

/* objToJSON */
PyObject* objToJSON(PyObject* self, PyObject *args, PyObject *kwargs);
void initObjToJSON(void);

/* JSONToObj */
PyObject* JSONToObj(PyObject* self, PyObject *args, PyObject *kwargs);

/* objToJSONFile */
PyObject* objToJSONFile(PyObject* self, PyObject *args, PyObject *kwargs);

/* JSONFileToObj */
PyObject* JSONFileToObj(PyObject* self, PyObject *args, PyObject *kwargs);


#define ENCODER_HELP_TEXT "Use ensure_ascii=false to output UTF-8. " \
    "Set encode_html_chars=True to encode < > & as unicode escape sequences. "\
    "Set escape_forward_slashes=False to prevent escaping / characters." \
    "Set allow_nan=False to raise an exception when NaN or Inf would be serialized." \
    "Set reject_bytes=True to raise TypeError on bytes."

static PyMethodDef ujsonMethods[] = {
  {"encode", (PyCFunction) objToJSON, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON. " ENCODER_HELP_TEXT},
  {"decode", (PyCFunction) JSONToObj, METH_VARARGS | METH_KEYWORDS, "Converts JSON as string to dict object structure."},
  {"dumps", (PyCFunction) objToJSON, METH_VARARGS | METH_KEYWORDS,  "Converts arbitrary object recursively into JSON. " ENCODER_HELP_TEXT},
  {"loads", (PyCFunction) JSONToObj, METH_VARARGS | METH_KEYWORDS,  "Converts JSON as string to dict object structure."},
  {"dump", (PyCFunction) objToJSONFile, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON file. " ENCODER_HELP_TEXT},
  {"load", (PyCFunction) JSONFileToObj, METH_VARARGS | METH_KEYWORDS, "Converts JSON as file to dict object structure."},
  {NULL, NULL, 0, NULL}       /* Sentinel */
};

static int ujson_exec(PyObject *module) {
  PyModule_AddStringConstant(module, "__version__", UJSON_VERSION);
  return 0;
}

static PyModuleDef_Slot ujson_slots[] = {
  {Py_mod_exec, ujson_exec},
  {0, NULL}
};

static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "ujson",
  0,              /* m_doc */
  NULL,             /* m_size */
  ujsonMethods,   /* m_methods */
  ujson_slots,    /* m_slots */
  NULL,           /* m_traverse */
  NULL,           /* m_clear */
  NULL            /* m_free */
};

PyObject *PyInit_ujson(void)
{
  initObjToJSON();
  return PyModuleDef_Init(&moduledef);
}
