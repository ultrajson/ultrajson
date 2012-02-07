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