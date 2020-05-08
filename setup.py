import os.path
import re
from glob import glob

from setuptools import Extension, setup

CLASSIFIERS = """
Development Status :: 5 - Production/Stable
Intended Audience :: Developers
License :: OSI Approved :: BSD License
Programming Language :: C
Programming Language :: Python :: 3
Programming Language :: Python :: 3.5
Programming Language :: Python :: 3.6
Programming Language :: Python :: 3.7
Programming Language :: Python :: 3.8
Programming Language :: Python :: 3 :: Only
"""

dconv_source_files = glob("./deps/double-conversion/double-conversion/*.cc")
dconv_source_files.append("./lib/dconv_wrapper.cc")

module1 = Extension(
    "ujson",
    sources=dconv_source_files
    + [
        "./python/ujson.c",
        "./python/objToJSON.c",
        "./python/JSONtoObj.c",
        "./lib/ultrajsonenc.c",
        "./lib/ultrajsondec.c",
    ],
    include_dirs=["./python", "./lib", "./deps/double-conversion/double-conversion"],
    extra_compile_args=["-D_GNU_SOURCE"],
    extra_link_args=["-lstdc++", "-lm"],
)


def get_version():
    filename = os.path.join(os.path.dirname(__file__), "./python/version.h")
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


f = open("README.rst")
try:
    README = f.read()
finally:
    f.close()


with open("python/version_template.h") as f:
    version_template = f.read()


def local_scheme(version):
    """Skip the local version (eg. +xyz of 0.6.1.dev4+gdf99fe2)
    to be able to upload to Test PyPI"""
    return ""


setup(
    name="ujson",
    description="Ultra fast JSON encoder and decoder for Python",
    long_description=README,
    ext_modules=[module1],
    author="Jonas Tarnstrom",
    download_url="https://github.com/ultrajson/ultrajson",
    platforms=["any"],
    url="https://github.com/ultrajson/ultrajson",
    project_urls={"Source": "https://github.com/ultrajson/ultrajson"},
    use_scm_version={
        "local_scheme": local_scheme,
        "write_to": "python/version.h",
        "write_to_template": version_template,
    },
    setup_requires=["setuptools_scm"],
    python_requires=">=3.5",
    classifiers=[x for x in CLASSIFIERS.split("\n") if x],
)
