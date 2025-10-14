"""
The definitions of the measurements we want to take
"""
import json

import scriptconfig as scfg
import ubelt as ub

from json_benchmarks import libraries


class MeasurementConfig(scfg.Config):
    default = {
        "disable": scfg.Value(
            [],
            choices=libraries.KNOWN_MODNAMES,
            help=ub.paragraph(
                """
                Remove specified libraries from the benchmarks
                """
            ),
        ),
        "factor": scfg.Value(
            1.0,
            help=ub.paragraph(
                """
                Specify as a fraction to speed up benchmarks for development /
                testing
                """
            ),
        ),
        "cache_dir": scfg.Value(
            None,
            help=ub.paragraph(
                """
                Location for benchmark cache.
                Defaults to $XDG_CACHE/ujson/benchmark_results/
                """
            ),
        ),
    }

    def normalize(self):
        dpath = self["cache_dir"]
        if dpath is None:
            dpath = ub.Path.appdir("ujson/benchmark_results")
        dpath = ub.Path(dpath)
        self["cache_dir"] = dpath


def benchmark_json():
    from json_benchmarks import benchmarker, datagen, libraries

    json_impls = libraries.available_json_impls()
    data_lut = datagen.json_test_data_generators()

    # These are the parameters that we benchmark over
    common_basis = {
        "impl": list(json_impls.keys()),
        "func": ["dumps", "loads"],
    }
    sized_basis = {
        "input": [
            "Array with doubles",
            "Array with UTF-8 strings",
            # 'Medium complex object',
            "Array with True values",
            "Array of Dict[str, int]",
            # 'Dict of List[Dict[str, int]]',
            # 'Complex object'
        ],
        "size": [1, 2, 4, 8, 16, 32, 128, 256, 512, 1024, 2048, 4096, 8192],
    }
    predefined_basis = {
        "input": ["Complex object"],
        "size": [None],
    }

    basis = [
        ub.dict_union(common_basis, predefined_basis),
        ub.dict_union(common_basis, sized_basis),
    ]

    # The Benchmarker class is a new experimental API around timerit to
    # abstract away the details of timing a process over a grid of parameters,
    # serializing the results, and aggregating results from disparate runs.
    benchmark = benchmarker.Benchmarker(
        name="bench_json",
        num=100,
        bestof=10,
        verbose=3,
        basis=basis,
    )

    def is_blocked(params):
        if params["input"] == "Complex object":
            # Some libraries can't handle the complex object
            if params["impl"] in {"orjson", "libpy_simdjson"}:
                return True

    # For each variation of your experiment, create a row.
    for params in benchmark.iter_params():
        if is_blocked(params):
            continue
        # Make any modifications you need to compute input kwargs for each
        # method here.
        impl_info = json_impls[params["impl"]]
        params["impl_version"] = impl_info["version"]
        method = impl_info[params["func"]]
        if method is None:
            # Not all libraries implement all methods
            continue
        py_data = data_lut[params["input"]](params["size"])
        if params["func"] == "dumps":
            data = py_data
        elif params["func"] == "loads":
            data = json.dumps(py_data)
        # Timerit will run some user-specified number of loops.
        # and compute time stats with similar methodology to timeit
        try:
            for timer in benchmark.measure():
                # Put any setup logic you dont want to time here.
                # ...
                with timer:
                    # Put the logic you want to time here
                    method(data)
        except Exception as ex:
            print(f"Failed to time: ex={ex}. Skipping")

    dpath = ub.Path.appdir("ujson/benchmark_results").ensuredir()
    result_fpath = benchmark.dump_in_dpath(dpath)
    return result_fpath
