import platform
from glob import glob

from setuptools import Extension, setup

dconv_source_files = glob("./deps/double-conversion/double-conversion/*.cc")
dconv_source_files.append("./lib/dconv_wrapper.cc")

strip_flags = ["-Wl,--strip-all"] if platform.system() == "Linux" else []

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
    extra_link_args=["-lstdc++", "-lm"] + strip_flags,
)

with open("python/version_template.h") as f:
    version_template = f.read()


def local_scheme(version):
    """Skip the local version (eg. +xyz of 0.6.1.dev4+gdf99fe2)
    to be able to upload to Test PyPI"""
    return ""


setup(
    ext_modules=[module1],
    use_scm_version={
        "local_scheme": local_scheme,
        "write_to": "python/version.h",
        "write_to_template": version_template,
    },
)
