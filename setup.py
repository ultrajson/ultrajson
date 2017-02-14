try:
  from setuptools import setup, Extension
except ImportError:
  from distutils.core import setup, Extension
import distutils.sysconfig
from distutils.sysconfig import customize_compiler
from distutils.command.build_clib import build_clib
from distutils.command.build_ext import build_ext
import os.path
import re
import sys
from glob import glob

CLASSIFIERS = filter(None, map(str.strip,
"""
Development Status :: 5 - Production/Stable
Intended Audience :: Developers
License :: OSI Approved :: BSD License
Programming Language :: C
Programming Language :: Python :: 2.6
Programming Language :: Python :: 2.7
Programming Language :: Python :: 3
Programming Language :: Python :: 3.2
Programming Language :: Python :: 3.4
""".splitlines()))

source_files = glob("./deps/double-conversion/double-conversion/*.cc")
source_files.append("./lib/dconv_wrapper.cc")

libdoubleconversion = (
    'double-conversion',
    dict(
        sources = source_files,
        include_dirs = ["./deps/double-conversion/double-conversion"],
        language = "c++"
    )
)


class build_clib_without_warnings(build_clib):
    def build_libraries(self, libraries):
        customize_compiler(self.compiler)

        try:
            self.compiler.compiler_so.remove("-Wstrict-prototypes")
        except (AttributeError, ValueError):
            pass

        build_clib.build_libraries(self, libraries)


module1 = Extension(
    'ujson',
     sources = [
         './python/ujson.c',
         './python/objToJSON.c',
         './python/JSONtoObj.c',
         './lib/ultrajsonenc.c',
         './lib/ultrajsondec.c'
     ],
     include_dirs = ['./python', './lib'],
     extra_compile_args = ['-D_GNU_SOURCE'],
     extra_link_args = ['-lstdc++', '-lm']
)


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


f = open('README.rst')
try:
    README = f.read()
finally:
    f.close()    
    

setup(
    name = 'ujson',
    version = get_version(),
    description = "Ultra fast JSON encoder and decoder for Python",
    long_description = README,
    libraries = [libdoubleconversion],
    ext_modules = [module1],
    author="Jonas Tarnstrom",
    author_email="jonas.tarnstrom@esn.me",
    download_url="http://github.com/esnme/ultrajson",
    license="BSD License",
    platforms=['any'],
    url="http://www.esn.me",
    cmdclass = {'build_ext': build_ext, 'build_clib': build_clib_without_warnings},
    classifiers=CLASSIFIERS,
)
