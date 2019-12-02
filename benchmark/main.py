import sys
CPYTHON = not hasattr(sys, 'pypy_version_info')
import time
import json
import ujson

from tests.support import import_ujson_universal
ujson_hpy_universal = import_ujson_universal()
ujson_hpy_universal.__name__ = 'ujson_hpy_universal'

if CPYTHON:
    import ujson_hpy


def benchmark(mod, fname, N):
    a = time.time()
    for i in range(N):
        with open(fname, 'rb') as f:
            for line in f:
                mod.loads(line)
    b = time.time()
    t = (b-a) * 1000.0 / N
    print('%20s: %6.2f ms per iteration [%2d iterations]' % (mod.__name__, t, N))


def main():
    N = 100
    fname = 'benchmark/2015-01-01-15.json'
    benchmark(json, fname, N)
    benchmark(ujson_hpy_universal, fname, N)
    benchmark(ujson, fname, N)
    if CPYTHON:
        benchmark(ujson_hpy, fname, N)


if __name__ == '__main__':
    main()
