"""
Main definition of the benchmarks
"""
import json

import scriptconfig as scfg
import ubelt as ub

from json_benchmarks import benchmarker, datagen
from json_benchmarks.benchmarker import util_stats

KNOWN_LIBRARIES = [
    "ujson",
    "nujson",
    "orjson",
    "simplejson",
    "json",
]


class JSONBenchmarkConfig(scfg.Config):
    """
    Benchmark JSON implementations
    """

    default = {
        "mode": scfg.Value(
            "all",
            position=1,
            choices=["all", "single", "run", "analyze"],
            help=ub.paragraph(
                """
                By default all benchmarks are run, saved, and aggregated
                with any other existing benchmarks for analysis and
                visualization.

                In "single" mode, other existing benchmarks are ignord.

                In "run" mode, the benchmarks are run, but no analysis is done.

                In "analyze" mode, no benchmarks are run, but any existing
                benchmarks are loaded for analysis and visualization.
                """
            ),
        ),
        "disable": scfg.Value(
            [],
            choices=KNOWN_LIBRARIES,
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


def available_json_impls():
    import importlib

    known_modnames = KNOWN_LIBRARIES
    json_impls = {}
    for libname in known_modnames:
        try:
            module = importlib.import_module(libname)
        except ImportError:
            pass
        else:
            json_impls[libname] = {
                "module": module,
                "version": module.__version__,
            }
    return json_impls


def benchmark_json():
    json_impls = available_json_impls()

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
        "size": [1, 2, 4, 8, 16, 32, 128, 256, 512],
        # 1024, 2048, 4096, 8192, 12288],
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
        num=1000,
        bestof=100,
        verbose=3,
        basis=basis,
    )

    def is_blocked(params):
        if params["input"] == "Complex object" and params["impl"] == "orjson":
            return True

    # For each variation of your experiment, create a row.
    for params in benchmark.iter_params():
        if is_blocked(params):
            continue
        # Make any modifications you need to compute input kwargs for each
        # method here.
        impl_info = json_impls[params["impl"]]
        params["impl_version"] = impl_info["version"]
        module = impl_info["module"]
        if params["func"] == "dumps":
            method = module.dumps
            data = data_lut[params["input"]](params["size"])
        elif params["func"] == "loads":
            method = module.loads
            to_encode = data_lut[params["input"]](params["size"])
            data = json.dumps(to_encode)
        # Timerit will run some user-specified number of loops.
        # and compute time stats with similar methodology to timeit
        for timer in benchmark.measure():
            # Put any setup logic you dont want to time here.
            # ...
            with timer:
                # Put the logic you want to time here
                method(data)

    dpath = ub.Path.appdir("ujson/benchmark_results").ensuredir()
    result_fpath = benchmark.dump_in_dpath(dpath)
    return result_fpath


def analyze_results(result_fpaths):
    import json

    results = []
    for fpath in result_fpaths:
        data = json.loads(fpath.read_text())
        for row in data["rows"]:
            result = benchmarker.BenchmarkerResult.load(fpath)
            results.extend(result.to_result_list())

    RECORD_ALL = 0
    metric_key = "time" if RECORD_ALL else "mean_time"

    # results = benchmark.result.to_result_list()

    analysis = benchmarker.result_analysis.ResultAnalysis(
        results,
        metrics=[metric_key],
        params=["impl"],
        metric_objectives={
            "min_time": "min",
            "mean_time": "min",
            "time": "min",
        },
    )
    analysis.analysis()

    table = analysis.table

    single_size = table[(table["size"] == 256) | table["size"].isnull()]
    # single_size_combo = aggregate_stats(single_size, None)
    single_size_combo = util_stats.aggregate_stats(
        single_size, suffix="_time", group_keys=["name"]
    )

    param_group = ["impl", "impl_version"]
    single_size_combo["calls/sec"] = 1 / single_size_combo["mean_time"]
    # _single_size_combo = single_size_combo.copy()
    # _single_size_combo["calls/sec"] = _single_size_combo["calls/sec"].apply(
    #
    # )
    time_piv = single_size_combo.pivot(["input", "func"], param_group, "mean_time")

    hz_piv = 1 / time_piv
    # hzstr_piv = (1 / time_piv).applymap(lambda x: f"{x:,.02f}")
    print("Table for size=256")
    # print(hzstr_piv.to_markdown())
    print(hz_piv.to_markdown(floatfmt=",.02f"))
    print("")
    print("Above metrics are in call/sec, larger is better.")

    speedup_piv = hz_piv / hz_piv["json"].values
    print(speedup_piv.to_markdown(floatfmt=",.02g"))

    analysis.abalate(param_group)
    # benchmark_analysis(rows, xlabel, group_labels, basis, RECORD_ALL)

    xlabel = "size"
    # Set these to empty lists if they are not used
    group_labels = {
        "fig": ["input"],
        "col": ["func"],
        # "fig": [],
        # "col": ["func" "input"],
        "hue": ["impl", "impl_version"],
        "size": [],
    }
    import kwplot

    kwplot.autosns()
    plots = analysis.plot(
        xlabel,
        metric_key,
        group_labels,
        xscale="log",
        yscale="log",
    )
    plots
    kwplot.show_if_requested()


def main(cmdline=True, **kwargs):
    """
    Example:
        >>> import sys, ubelt
        >>> sys.path.append(ubelt.expandpath('~/code/ultrajson'))
        >>> from json_benchmarks.core import *  # NOQA
        >>> import kwplot
        >>> kwplot.autosns()
        >>> cmdline = False
        >>> kwargs = {}
        >>> main(cmdline, **kwargs)
    """
    config = JSONBenchmarkConfig(cmdline=cmdline, data=kwargs)
    dpath = config["cache_dir"]

    run = config["mode"] in {"all", "single", "run"}
    if run:
        result_fpath = benchmark_json()
        print(f"result_fpath = {result_fpath!r}")
        result_fpaths = [result_fpath]

    agg = config["mode"] not in {"single"}
    if agg:
        result_fpaths = list(dpath.glob("benchmarks*.json"))

    analyze = config["mode"] in {"all", "single", "analyze"}
    if analyze:
        analyze_results(result_fpaths)
