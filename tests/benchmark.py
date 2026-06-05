# coding=UTF-8
import json
import os
import platform
import random
import sys
import timeit
from collections import defaultdict

import ujson

# Will be set by "main" if user requests them
simplejson = None
orjson = None

USER = {
    "userId": 3381293,
    "age": 213,
    "username": "johndoe",
    "fullname": "John Doe the Second",
    "isAuthorized": True,
    "liked": 31231.31231202,
    "approval": 31.1471,
    "jobs": [1, 2],
    "currJob": None,
}
FRIENDS = [USER, USER, USER, USER, USER, USER, USER, USER]

decode_data = None
test_object = None

benchmark_results = []


benchmark_registry = defaultdict(dict)


# =============================================================================
# Benchmark registration
# =============================================================================


def register_benchmark(libname, testname):
    def _wrap(func):
        benchmark_registry[testname][libname] = func
        return func

    return _wrap


# =============================================================================
# Logging benchmarking results.
# =============================================================================
def results_new_benchmark(name):
    benchmark_results.append((name, {}, {}))
    print(name)


def results_record_result(callback, is_encode, count):
    callback_name = callback.__name__
    library = callback_name.split("_")[-1]
    try:
        results = timeit.repeat(
            f"{callback_name}()",
            f"from __main__ import {callback_name}",
            repeat=10,
            number=count,
        )
    except (TypeError, json.decoder.JSONDecodeError):
        return
    result = count / min(results)
    benchmark_results[-1][1 if is_encode else 2][library] = result

    print(
        "{} {}: {:,.02f} calls/sec".format(
            library, "encode" if is_encode else "decode", result
        )
    )


def results_output_table(libraries):
    uname_system, _, uname_release, uname_version, _, uname_processor = platform.uname()
    print()
    print("### Test machine")
    print()
    print(uname_system, uname_release, uname_processor, uname_version)
    print()
    print("### Versions")
    print()
    print(
        "- {} {}".format(
            platform.python_implementation(), sys.version.replace("\n", "")
        )
    )
    for libname in libraries:
        module = globals()[libname]
        print(f"- {libname:<12} : {module.__version__}")
    print()

    column_widths = [max(len(r[0]) for r in benchmark_results)]
    for library in libraries:
        column_widths.append(max(10, len(library)))

    columns = [" " * (width + 2) for width in column_widths]
    for i, library in enumerate(libraries):
        columns[i + 1] = (" " + library).ljust(column_widths[i + 1] + 2)
    print("|{}|".format("|".join(columns)))

    line = (
        "|"
        + "-" * (column_widths[0] + 2)
        + "|"
        + ":|".join("-" * (width + 1) for width in column_widths[1:])
        + ":|"
    )
    print(line)

    for name, encodes, decodes in benchmark_results:
        columns = [" " * (width + 2) for width in column_widths]
        columns[0] = (" " + name).ljust(column_widths[0] + 2)
        print("|{}|".format("|".join(columns)))

        columns = [None] * len(column_widths)
        columns[0] = " encode".ljust(column_widths[0] + 2)
        for i, library in enumerate(libraries):
            if library in encodes:
                columns[i + 1] = f"{encodes[library]:,.0f} ".rjust(
                    column_widths[i + 1] + 2
                )
            else:
                columns[i + 1] = " " * (column_widths[i + 1] + 2)
        print("|{}|".format("|".join(columns)))

        if decodes:
            columns = [None] * len(column_widths)
            columns[0] = " decode".ljust(column_widths[0] + 2)
            for i, library in enumerate(libraries):
                if library in decodes:
                    columns[i + 1] = f"{decodes[library]:,.0f} ".rjust(
                        column_widths[i + 1] + 2
                    )
                else:
                    columns[i + 1] = " " * (column_widths[i + 1] + 2)
            print("|{}|".format("|".join(columns)))

    print()
    print("Above metrics are in call/sec, larger is better.")


# =============================================================================
# JSON encoding.
# =============================================================================

_testname = "dumps"


@register_benchmark("json", _testname)
def dumps_with_json():
    json.dumps(test_object)


@register_benchmark("orjson", _testname)
def dumps_with_orjson():
    orjson.dumps(test_object)


@register_benchmark("simplejson", _testname)
def dumps_with_simplejson():
    simplejson.dumps(test_object)


@register_benchmark("ujson", _testname)
def dumps_with_ujson():
    ujson.dumps(test_object, ensure_ascii=False)


# =============================================================================
# JSON encoding with sort_keys=True.
# =============================================================================

_testname = "dumps-sort_keys=True"


@register_benchmark("json", _testname)
def dumps_sorted_with_json():
    json.dumps(test_object, sort_keys=True)


@register_benchmark("simplejson", _testname)
def dumps_sorted_with_simplejson():
    simplejson.dumps(test_object, sort_keys=True)


@register_benchmark("orjson", _testname)
def dumps_sorted_with_orjson():
    orjson.dumps(test_object, sort_keys=True)


@register_benchmark("ujson", _testname)
def dumps_sorted_with_ujson():
    ujson.dumps(test_object, ensure_ascii=False, sort_keys=True)


# =============================================================================
# JSON decoding.
# =============================================================================

_testname = "loads"


@register_benchmark("json", _testname)
def loads_with_json():
    json.loads(decode_data)


@register_benchmark("orjson", _testname)
def loads_with_orjson():
    orjson.loads(decode_data)


@register_benchmark("simplejson", _testname)
def loads_with_simplejson():
    simplejson.loads(decode_data)


@register_benchmark("ujson", _testname)
def loads_with_ujson():
    ujson.loads(decode_data)


# =============================================================================
# Benchmarks.
# =============================================================================
def run_decode(count, libraries):
    _testname = "loads"
    for libname in libraries:
        func = benchmark_registry[_testname][libname]
        results_record_result(func, False, count)


def run_encode(count, libraries):
    _testname = "dumps"
    for libname in libraries:
        func = benchmark_registry[_testname][libname]
        results_record_result(func, True, count)


def run_encode_sort_keys(count, libraries):
    _testname = "dumps-sort_keys=True"
    for libname in libraries:
        func = benchmark_registry[_testname][libname]
        results_record_result(func, True, count)


def benchmark_array_doubles(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Array with 256 doubles")
    COUNT = max(int(10000 * factor), 1)

    test_object = []
    for x in range(256):
        test_object.append(sys.maxsize * random.random())
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    test_object = None
    run_decode(COUNT, libraries)

    decode_data = None


def benchmark_array_utf8_strings(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Array with 256 UTF-8 strings")
    COUNT = max(int(2000 * factor), 1)

    test_object = []
    for x in range(256):
        test_object.append(
            "نظام الحكم سلطاني وراثي "
            "في الذكور من ذرية السيد تركي بن سعيد بن سلطان ويشترط فيمن يختار لولاية"
            " الحكم من بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين "
        )
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    test_object = None
    run_decode(COUNT, libraries)

    decode_data = None


def benchmark_array_byte_strings(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Array with 256 strings")
    COUNT = max(int(10000 * factor), 1)

    test_object = []
    for x in range(256):
        test_object.append("A pretty long string which is in a list")
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    test_object = None
    run_decode(COUNT, libraries)

    decode_data = None


def benchmark_medium_complex_object(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Medium complex object")
    COUNT = max(int(5000 * factor), 1)

    test_object = [
        [USER, FRIENDS],
        [USER, FRIENDS],
        [USER, FRIENDS],
        [USER, FRIENDS],
        [USER, FRIENDS],
        [USER, FRIENDS],
    ]
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    test_object = None
    run_decode(COUNT, libraries)

    decode_data = None


def benchmark_array_true_values(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Array with 256 True values")
    COUNT = max(int(50000 * factor), 1)

    test_object = []
    for x in range(256):
        test_object.append(True)
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    test_object = None
    run_decode(COUNT, libraries)

    decode_data = None


def benchmark_array_of_dict_string_int_pairs(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Array with 256 dict{string, int} pairs")
    COUNT = max(int(5000 * factor), 1)

    test_object = []
    for x in range(256):
        test_object.append({str(random.random() * 20): int(random.random() * 1000000)})
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    test_object = None
    run_decode(COUNT, libraries)

    decode_data = None


def benchmark_dict_of_arrays_of_dict_string_int_pairs(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Dict with 256 arrays with 256 dict{string, int} pairs")
    COUNT = max(int(50 * factor), 1)

    test_object = {}
    for y in range(256):
        arrays = []
        for x in range(256):
            arrays.append({str(random.random() * 20): int(random.random() * 1000000)})
        test_object[str(random.random() * 20)] = arrays
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    run_decode(COUNT, libraries)

    decode_data = None

    results_new_benchmark(
        "Dict with 256 arrays with 256 dict{string, int} pairs, outputting sorted keys"
    )
    run_encode_sort_keys(COUNT, libraries)

    test_object = None


def benchmark_complex_object(libraries, factor=1):
    global decode_data, test_object
    results_new_benchmark("Complex object")
    COUNT = int(100 * factor)

    with open(os.path.join(os.path.dirname(__file__), "sample.json")) as f:
        test_object = json.load(f)
    run_encode(COUNT, libraries)

    decode_data = json.dumps(test_object)
    test_object = None
    run_decode(COUNT, libraries)

    decode_data = None


# =============================================================================
# Main.
# =============================================================================


def main():
    import argparse
    import importlib

    parser = argparse.ArgumentParser(
        prog="ujson-benchmarks",
        description="Benchmark ujson against other json implementations",
    )

    known_libraries = [
        "ujson",
        "orjson",
        "simplejson",
        "json",
    ]

    parser.add_argument(
        "--disable",
        nargs="+",
        choices=known_libraries,
        help="Remove specified libraries from the benchmarks",
        default=[],
    )

    parser.add_argument(
        "--factor",
        type=float,
        default=1.0,
        help="Specify as a fraction speed up benchmarks for development / testing",
    )

    args = parser.parse_args()

    disabled_libraries = set(args.disable)
    enabled_libraries = {}
    for libname in known_libraries:
        if libname not in disabled_libraries:
            try:
                module = importlib.import_module(libname)
            except ImportError:
                raise ImportError(f"{libname} is not available")
            else:
                enabled_libraries[libname] = module

    # Ensure the modules are available in the global scope
    for libname, module in enabled_libraries.items():
        print(f"Enabled {libname} benchmarks")
        globals()[libname] = module

    libraries = list(enabled_libraries.keys())
    factor = args.factor
    benchmark_array_doubles(libraries, factor)
    benchmark_array_utf8_strings(libraries, factor)
    benchmark_array_byte_strings(libraries, factor)
    benchmark_medium_complex_object(libraries, factor)
    benchmark_array_true_values(libraries, factor)
    benchmark_array_of_dict_string_int_pairs(libraries, factor)
    benchmark_dict_of_arrays_of_dict_string_int_pairs(libraries, factor)
    benchmark_complex_object(libraries, factor)

    results_output_table(libraries)


if __name__ == "__main__":
    main()
