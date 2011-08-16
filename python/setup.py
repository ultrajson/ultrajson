from distutils.core import setup, Extension
import distutils.sysconfig
import shutil

try:
	shutil.rmtree("./build")
except(OSError):
	pass

module1 = Extension('ujson',
                    sources = ['ujson.c', 'objToJSON.c', 'JSONtoObj.c', '../ultrajsonenc.c', '../ultrajsondec.c'],
                    include_dirs = ['../'])
					
setup (name = 'ujson',
		version = '1.5',
		description = 'Ultra fast JSON encoder and decoder for Python',
		ext_modules = [module1],
		author = "Jonas Tarnstrom",
		author_email = "jonas.tarnstrom@esn.me",
		maintainer = "Jonas Tarnstrom",
		maintainer_email = "jonas.tarnstrom@esn.me",
		license = "BSD")

