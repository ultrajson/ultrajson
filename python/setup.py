from distutils.core import setup, Extension
import distutils.sysconfig

module1 = Extension('ujson',
                    sources = ['ujson.c', 'objToJSON.c'],
                    include_dirs = ['../'],
					library_dirs = ['./lib/'],
					libraries=['libultrajson'])
					
setup (name = 'ujson',
		version = '1.0',
		description = 'UltraJSON',
		ext_modules = [module1],
		data_files = [(distutils.sysconfig.get_python_lib(), ["./lib/libultrajson.dll"])])

