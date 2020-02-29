import sys
from pathlib import Path
import hpy.universal

def import_ujson_universal():
    """
    Find and import ujson.hpy.so.

    Eventually this logic should be provided directly by hpy_universal,
    but in the meantime we just do it manually.
    """
    # find ujson_hpy.hpy.so in sys.path
    for p in sys.path:
        sopath = Path(p) / 'ujson_hpy.hpy.so'
        if sopath.exists():
            break
    else:
        raise ImportError('Cannot find ujson.hpy.so')
    #
    # load it through hpy_universal
    class Spec:
        name = 'ujson_hpy'
        origin = str(sopath)
    return hpy.universal.load_from_spec(Spec)
