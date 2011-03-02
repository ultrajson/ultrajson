from distutils.core import setup, Extension
import distutils.sysconfig
import shutil

try:
	shutil.rmtree("./build")
except(OSError):
	pass

module1 = Extension('ujson',
                    sources = ['ujson.c', 'objToJSON.c', 'JSONToObj.c'],
                    include_dirs = ['../'],
					library_dirs = ['./lib/'],
					libraries=['ujson'])
					
setup (name = 'ujson',
		version = '1.0',
		description = 'UltraJSON',
		ext_modules = [module1],
		author = "Jonas Tarnstrom",
		author_email = "jonas.tarnstrom@esn.me",
		maintainer = "Jonas Tarnstrom",
		maintainer_email = "jonas.tarnstrom@esn.me",
		license = "BSD")

