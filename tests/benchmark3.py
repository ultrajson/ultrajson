"""
Roadmap:

    - [ ]
"""
import random
import sys
import ubelt as ub


def data_lut(input, size):
    if input == "Array with UTF-8 strings":
        test_object = []
        for x in range(size):
            test_object.append(
                "نظام الحكم سلطاني وراثي "
                "في الذكور من ذرية السيد تركي بن سعيد بن سلطان ويشترط فيمن يختار لولاية"
                " الحكم من بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين "
            )
        return test_object
    elif input == "Array with doubles":
        test_object = []
        for x in range(256):
            test_object.append(sys.maxsize * random.random())
    else:
        raise KeyError(input)


def available_json_impls():
    JSON_IMPLS = {}

    try:
        import json
        JSON_IMPLS["json"] = json
    except ImportError:
        pass

    try:
        import ujson
        JSON_IMPLS["ujson"] = ujson
    except ImportError:
        pass

    try:
        import nujson
        JSON_IMPLS["nujson"] = nujson
    except ImportError:
        pass

    try:
        import orjson
        JSON_IMPLS["nujson"] = orjson
    except ImportError:
        pass

    try:
        import simplejson
        JSON_IMPLS["simplejson"] = simplejson
    except ImportError:
        pass

    return JSON_IMPLS


def benchmark_json_dumps():
    # TODO: remove this hack
    sys.path.append(ub.expandpath('~/code/ultrajson/tests'))
    from benchmarker import Benchmarker

    JSON_IMPLS = available_json_impls()

    version_infos = {k: v.__version__ for k, v in JSON_IMPLS.items()}

    def method_lut(impl):
        return JSON_IMPLS[impl].dumps

    # These are the parameters that we benchmark over
    basis = {
        "input": [
            "Array with UTF-8 strings",
            "Array with doubles",
        ],
        "size": [1, 32, 256, 1024, 2048],
        "impl": list(JSON_IMPLS.keys()),
    }

    benchmark = Benchmarker(
        name='bench_json_dumps',
        # Change params here to modify number of trials
        num=100,
        bestof=10,
        basis=basis,
    )

    # For each variation of your experiment, create a row.
    for params in benchmark.iter_params():
        # Make any modifications you need to compute input kwargs for each
        # method here.
        impl = params["impl"]
        impl_version = version_infos[impl]
        params["impl_version"] = impl_version
        method = method_lut(impl)
        data = data_lut(params["input"], params["size"])
        # Timerit will run some user-specified number of loops.
        # and compute time stats with similar methodology to timeit
        for timer in benchmark.measure():
            # Put any setup logic you dont want to time here.
            # ...
            with timer:
                # Put the logic you want to time here
                method(data)

    dpath = ub.Path.appdir('ujson/benchmark_results').ensuredir()
    benchmark.dump_in_dpath(dpath)

    RECORD_ALL = 0
    metric_key = "time" if RECORD_ALL else "mean"

    from benchmarker import result_analysis
    results = benchmark.result.to_result_list()
    analysis = result_analysis.ResultAnalysis(
        results,
        metrics=[metric_key],
        params=['impl'],
        metric_objectives={
            'min': 'min',
            'mean': 'min',
            'time': 'min',
        })
    analysis.analysis()

    # benchmark_analysis(rows, xlabel, group_labels, basis, RECORD_ALL)


if __name__ == "__main__":
    """
    CommandLine:
        python ~/code/ultrajson/tests/benchmark3.py
    """
    benchmark_json_dumps()
