"""
A brute force fuzzer for detecting memory issues in ujson.dumps(). To use, first
compile ujson in debug mode:

    CFLAGS='-DDEBUG' python setup.py -q build_ext --inplace -f

Then run without arguments:

    python tests/fuzz.py

If it crashes, the last line of output is the arguments to reproduce the
failure.

    python tests/fuzz.py {{ last line of output before crash }}

Adding --dump-python or --dump-json will print the object it intends to
serialise as either a Python literal or in JSON.

"""

import argparse
import gc
import itertools
import json
import math
import random
import re
import sys
from pprint import pprint

import ujson


class FuzzGenerator:
    """A random JSON serialisable object generator."""

    def __init__(self, seed=None):
        self._randomizer = random.Random(seed)
        self._shrink = 1

    def key(self):
        key_types = [self.int, self.float, self.string, self.null, self.bool]
        return self._randomizer.choice(key_types)()

    def item(self):
        if self._randomizer.random() > 0.8:
            return self.key()
        return self._randomizer.choice([self.list, self.dict])()

    def int(self):
        return int(self.float())

    def float(self):
        sign = self._randomizer.choice([-1, 1, 0])
        return sign * math.exp(self._randomizer.uniform(-40, 40))

    def string(self):
        characters = ["\x00", "\t", "a", "\U0001f680", "<></>", "\u1234"]
        return self._randomizer.choice(characters) * self.length()

    def bool(self):
        return self._randomizer.random() < 0.5

    def null(self):
        return None

    def list(self):
        return [self.item() for i in range(self.length())]

    def dict(self):
        return {self.key(): self.item() for i in range(self.length())}

    def length(self):
        self._shrink *= 0.99
        return int(math.exp(self._randomizer.uniform(-0.5, 5)) * self._shrink)


def random_object(seed=None):
    return FuzzGenerator(seed).item()


class RangeOption(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        values = re.findall("[^: ]+", values)
        if len(values) == 1:
            values = (int(values[0]),)
        else:
            values = range(*map(int, values))
        setattr(namespace, self.dest, values)


class ListOption(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        values = tuple(map(int, re.findall("[^, ]+", values)))
        setattr(namespace, self.dest, values)


parser = argparse.ArgumentParser(
    epilog=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
)
parser.add_argument(
    "--seed",
    default=range(100),
    action=RangeOption,
    dest="seeds",
    help="A seed or range of seeds (in the form start:end[:step]) "
    "to initialise the randomizer.",
)
parser.add_argument(
    "--indent",
    default=(0, 1, 2, 3, 4, 5, 12, 100, 1000),
    action=ListOption,
    help="A comma separated sequence of indentation lengths to test.",
)
parser.add_argument(
    "--ensure_ascii",
    default=(0, 1),
    action=ListOption,
    help="Sets the ensure_ascii option to ujson.dumps(). "
    "May be 0 or 1 or 0,1 to testboth.",
)
parser.add_argument(
    "--encode_html_chars",
    default=(0, 1),
    action=ListOption,
    help="Sets the encode_html_chars option to ujson.dumps(). "
    "May be 0 or 1 or 0,1 to test both.",
)
parser.add_argument(
    "--escape_forward_slashes",
    default=(0, 1),
    action=ListOption,
    help="Sets the escape_forward_slashes option to ujson.dumps(). "
    "May be 0 or 1 or 0,1 to test both.",
)
parser.add_argument(
    "--dump-python",
    action="store_true",
    help="Print the randomly generated object as a Python literal and exit.",
)
parser.add_argument(
    "--dump-json",
    action="store_true",
    help="Print the randomly generated object in JSON format and exit.",
)


def cli(args=None):
    options = dict(parser.parse_args(args)._get_kwargs())
    if options.pop("dump_json"):
        print(json.dumps(random_object(options["seeds"][0]), indent=2))
    elif options.pop("dump_python"):
        pprint(random_object(options["seeds"][0]))
    else:
        fuzz(**options)


def fuzz(seeds, **options):
    try:
        for seed in seeds:
            data = random_object(seed)
            for permutation in itertools.product(*options.values()):
                _options = dict(zip(options.keys(), permutation))
                print(f"--seed {seed}", *(f"--{k} {v}" for (k, v) in _options.items()))

                data_objects = collect_all_objects(data)
                # Exclude ints because they get referenced by the lists below.
                data_objects = [o for o in data_objects if not isinstance(o, int)]
                gc.collect()
                data_ref_counts_before = [sys.getrefcount(o) for o in data_objects]
                ujson.dumps(data, **_options)
                gc.collect()
                data_ref_counts_after = [sys.getrefcount(o) for o in data_objects]
                if data_ref_counts_before != data_ref_counts_after:
                    for o, before, after in zip(
                        data_objects, data_ref_counts_before, data_ref_counts_after
                    ):
                        if before != after:
                            print(f"Ref count of {o!r} went from {before} to {after}")
                    raise ValueError("ref counts changed")
    except KeyboardInterrupt:
        pass


def collect_all_objects(obj):
    """Given an object, return a list of all objects referenced by it."""

    if hasattr(sys, "pypy_version_info"):
        # PyPy's GC works differently (no ref counting), so this wouldn't be useful.
        # Simply returning an empty list effectively disables the refcount test.
        return []

    def _inner(o):
        yield o
        if isinstance(o, list):
            for v in o:
                yield from _inner(v)
        elif isinstance(o, dict):
            for k, v in o.items():
                yield from _inner(k)
                yield from _inner(v)

    out = []
    seen = set()
    for o in _inner(obj):
        if id(o) not in seen:
            seen.add(id(o))
            out.append(o)
    return out


if __name__ == "__main__":
    cli()
