"""
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
"""

from distutils.core import setup, Extension
import distutils.sysconfig
import shutil
import os.path
import re

CLASSIFIERS = filter(None, map(str.strip,
"""
Development Status :: 5 - Production/Stable
Intended Audience :: Developers
License :: OSI Approved :: BSD License
Programming Language :: C
Programming Language :: Python :: 2.4
Programming Language :: Python :: 2.5
Programming Language :: Python :: 2.6
Programming Language :: Python :: 2.7
""".splitlines()))

try:
	shutil.rmtree("./build")
except(OSError):
	pass

module1 = Extension('ujson',
                    sources = ['./python/ujson.c', './python/objToJSON.c', './python/JSONtoObj.c', './lib/ultrajsonenc.c', './lib/ultrajsondec.c'],
                    include_dirs = ['./python', './lib'])

def get_version():
	filename = os.path.join(os.path.dirname(__file__), './python/version.h')
	file = None
	try:
		file = open(filename)
		header = file.read()
	finally:
		if file:
			file.close()
	m = re.search(r'#define\s+UJSON_VERSION\s+"(\d+\.\d+(?:\.\d+)?)"', header)
	assert m, "version.h must contain UJSON_VERSION macro"
	return m.group(1)

setup (name = 'ujson',
       version = get_version(),
       description = "Ultra fast JSON encoder and decoder for Python",
       ext_modules = [module1],
       author="Jonas Tarnstrom",
       author_email="jonas.tarnstrom@esn.me",
       download_url="http://github.com/esnme/ultrajson",
       license="BSD License",
       platforms=['any'],	   
	   url="http://www.esn.me",
       classifiers=CLASSIFIERS,
	   )