"""
Define the json libraries we are considering
"""

KNOWN_LIBRARIES = [
    {'modname': "ujson", 'distname': 'ujson'},
    {'modname': "nujson", 'distname': 'nujson'},
    {'modname': "orjson", 'distname': 'orjson'},
    {'modname': "simplejson", 'distname': 'simplejson'},
    {'modname': "json", 'distname': "<stdlib>"},
    {'modname': "simdjson", 'distname': 'pysimdjson'},
]

KNOWN_MODNAMES = [info['modname'] for info in KNOWN_LIBRARIES]


# TODO:
# def distname_to_modnames(distname):
#     # TODO: nice way to switch between a module's import name and it's distribution name
#     # References:
#     # https://stackoverflow.com/questions/49764802/get-module-name-programmatically-with-only-pypi-package-name/49764960#49764960
#     import distlib.database
#     distlib.database.DistributionPath().get_distribution(distname)
#     # import importlib.metadata
#     # importlib.metadata.metadata(distname)
#     # importlib.util.find_spec(modname)
#     # import simdjson
#     # import pkg_resources
#     # pkg_resources.get_distribution('pysimdjson')


def available_json_impls():
    """
    Return a dictionary of information about each json implementation

    Example:
        >>> from json_benchmarks.libraries import *  # NOQA
        >>> json_impls = available_json_impls()
        >>> print('json_impls = {}'.format(ub.repr2(json_impls, nl=1)))
    """
    import importlib
    known_libinfo = KNOWN_LIBRARIES
    json_impls = {}
    for libinfo in known_libinfo:
        modname = libinfo['modname']
        distname = libinfo['distname']
        try:
            module = importlib.import_module(modname)
        except ImportError:
            pass
        else:
            import pkg_resources
            mod_version = getattr(module, '__version__', None)
            if distname == '<stdlib>':
                pkg_version = mod_version
            else:
                pkg_version = pkg_resources.get_distribution(distname).version
            if mod_version is not None:
                assert mod_version == pkg_version
            version = pkg_version
            json_impls[modname] = {
                "module": module,
                "modname": modname,
                "distname": distname,
                "version": version,
            }
    return json_impls
