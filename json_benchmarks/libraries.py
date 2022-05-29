"""
Define the json libraries we are considering
"""

KNOWN_LIBRARIES = [
    {"modname": "ujson", "distname": "ujson"},
    {"modname": "nujson", "distname": "nujson"},
    {"modname": "orjson", "distname": "orjson"},
    {"modname": "simplejson", "distname": "simplejson"},
    {"modname": "json", "distname": "<stdlib>"},
    {"modname": "simdjson", "distname": "pysimdjson"},
    {"modname": "cysimdjson", "distname": "cysimdjson"},
    {"modname": "libpy_simdjson", "distname": "libpy-simdjson"},
]

KNOWN_MODNAMES = [info["modname"] for info in KNOWN_LIBRARIES]


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


class Compatability:
    """
    Expose a common API for all tested implmentations
    """

    @staticmethod
    def lut_dumps(module):
        if module.__name__ == "cysimdjson":
            return None
        elif module.__name__ == 'simdjson':
            return None
        else:
            return getattr(module, "dumps", None)

    @staticmethod
    def lut_loads(module):
        if module.__name__ == "cysimdjson":
            parser = module.JSONParser()
            return parser.loads
        else:
            return getattr(module, "loads", None)


def available_json_impls():
    """
    Return a dictionary of information about each json implementation

    Example:
        >>> from json_benchmarks.libraries import *  # NOQA
        >>> json_impls = available_json_impls()
        >>> print('json_impls = {}'.format(ub.repr2(json_impls, nl=1)))
    """
    import importlib

    import pkg_resources

    known_libinfo = KNOWN_LIBRARIES
    json_impls = {}
    for libinfo in known_libinfo:
        modname = libinfo["modname"]
        distname = libinfo["distname"]
        try:
            module = importlib.import_module(modname)
        except ImportError:
            pass
        else:
            mod_version = getattr(module, "__version__", None)
            if distname == "<stdlib>":
                pkg_version = mod_version
            else:
                pkg_version = pkg_resources.get_distribution(distname).version
            if mod_version is not None:
                assert mod_version == pkg_version
            version = pkg_version
            dumps = Compatability.lut_dumps(module)
            loads = Compatability.lut_loads(module)
            impl_info = {
                "module": module,
                "modname": modname,
                "distname": distname,
                "version": version,
                "dumps": dumps,
                "loads": loads,
            }
            json_impls[modname] = impl_info
    return json_impls
