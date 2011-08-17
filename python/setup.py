from distutils.core import setup, Extension
import distutils.sysconfig
import shutil
import os.path
import re

try:
	shutil.rmtree("./build")
except(OSError):
	pass

module1 = Extension('ujson',
                    sources = ['ujson.c', 'objToJSON.c', 'JSONtoObj.c', 'ultrajsonenc.c', 'ultrajsondec.c'],
                    headers = ['version.h'],
                    include_dirs = ['./'])

def get_version():
	filename = os.path.join(os.path.dirname(__file__), 'version.h')
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
		description = 'Ultra fast JSON encoder and decoder for Python',
		ext_modules = [module1],
		author = "Jonas Tarnstrom",
		author_email = "jonas.tarnstrom@esn.me",
		maintainer = "Jonas Tarnstrom",
		maintainer_email = "jonas.tarnstrom@esn.me",
		license = "BSD")

