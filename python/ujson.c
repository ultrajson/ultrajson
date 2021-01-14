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

#include "py_defines.h"
#include "version.h"

/* objToJSON */
PyObject* objToJSON(PyObject* self, PyObject *args, PyObject *kwargs);
void initObjToJSON(void);

/* objToJSONFile */
PyObject* objToJSONFile(PyObject* self, PyObject *args, PyObject *kwargs);

extern HPyDef JSONToObj;
extern HPyDef JSONToObj_decode;
extern HPyDef JSONFileToObj;

static HPyDef *module_defines[] = {
    &JSONToObj,
    &JSONToObj_decode,
    &JSONFileToObj,
    NULL
};

#define ENCODER_HELP_TEXT "Use ensure_ascii=false to output UTF-8. Pass in double_precision to alter the maximum digit precision of doubles. Set encode_html_chars=True to encode < > & as unicode escape sequences. Set escape_forward_slashes=False to prevent escaping / characters."

static PyMethodDef ujsonMethods[] = {
  {"encode", (PyCFunction) objToJSON, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON. " ENCODER_HELP_TEXT},
  {"dumps", (PyCFunction) objToJSON, METH_VARARGS | METH_KEYWORDS,  "Converts arbitrary object recursively into JSON. " ENCODER_HELP_TEXT},
  {"dump", (PyCFunction) objToJSONFile, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON file. " ENCODER_HELP_TEXT},
  {NULL, NULL, 0, NULL}       /* Sentinel */
};

static HPyModuleDef moduledef = {
  HPyModuleDef_HEAD_INIT,
  .m_name = "ujson_hpy",
  .m_doc = 0,
  .m_size = -1,
  .defines = module_defines,
  .legacy_methods = ujsonMethods,
};


HPy_MODINIT(ujson_hpy)
static HPy init_ujson_hpy_impl(HPyContext ctx)
{
  HPy module;
  //PyObject *version_string;

  initObjToJSON();
  module = HPyModule_Create(ctx, &moduledef);
  if (HPy_IsNull(module)) {
      return HPy_NULL;
  }

  //version_string = PyUnicode_FromString (UJSON_VERSION);
  //PyModule_AddObject (module, "__version__", version_string);
  return module;
}
