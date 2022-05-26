"""
Roadmap:

    - [ ]
"""
import random
import sys
import ubelt as ub


def json_test_data_generators():
    """
    Generates data for benchmarks with various sizes

    Returns:
        Dict[str, callable]:
            a mapping from test data name to its generator

    Example:
        >>> data_lut = json_test_data_generators()
        >>> size = 2
        >>> keys = sorted(set(data_lut) - {'Complex object'})
        >>> for key in keys:
        >>>     func = data_lut[key]
        >>>     test_object = func(size)
        >>>     print('key = {!r}'.format(key))
        >>>     print('test_object = {!r}'.format(test_object))
    """
    data_lut = {}
    def _register_data(name):
        def _wrap(func):
            data_lut[name] = func
        return _wrap

    # seed if desired
    #rng = random.Random()
    rng = random

    @_register_data('Array with doubles')
    def array_with_doubles(size):
        test_object = [sys.maxsize * rng.random() for _ in range(size)]
        return test_object

    @_register_data('Array with UTF-8 strings')
    def array_with_utf8_strings(size):
        utf8_string = (
            "نظام الحكم سلطاني وراثي "
            "في الذكور من ذرية السيد تركي بن سعيد بن سلطان ويشترط فيمن يختار لولاية"
            " الحكم من بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين "
        )
        test_object = [utf8_string for _ in range(size)]
        return test_object

    @_register_data('Medium complex object')
    def medium_complex_object(size):
        user = {
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
        friends = [user, user, user, user, user, user, user, user]
        test_object = [[user, friends] for _ in range(size)]
        return test_object

    @_register_data('Array with True values')
    def true_values(size):
        test_object = [True for _ in range(size)]
        return test_object

    @_register_data('Array of Dict[str, int]')
    def array_of_dict_string_int(size):
        test_object = [
            {str(rng.random() * 20): int(rng.random() * 1000000)}
            for _ in range(size)
        ]
        return test_object

    @_register_data('Dict of List[Dict[str, int]]')
    def dict_of_list_dict_str_int(size):
        keys = set()
        while len(keys) < size:
            key = str(rng.random() * 20)
            keys.add(key)
        test_object = {
            key: [
                {str(rng.random() * 20): int(rng.random() * 1000000)}
                for _ in range(256)
            ]
            for key in keys
        }
        return test_object

    @_register_data('Complex object')
    def complex_object(size):
        import json
        # TODO: might be better to reigster this file with setup.py or
        # download it via some mechanism
        try:
            dpath = ub.Path(__file__).parent
            fpath = dpath / 'sample.json'
            if not fpath.exists():
                raise Exception
        except Exception:
            import ujson
            dpath = ub.Path(ujson.__file__).parent / 'tests'
            fpath = dpath / 'sample.json'
            if not fpath.exists():
                raise Exception
        with open(fpath, 'r') as f:
            test_object = json.load(f)
        if size > 1:
            test_object = [test_object] * size
        return test_object

    return data_lut


def available_json_impls():
    import importlib
    known_modnames = [
        'ujson', 'json', 'nujson', 'orjson', 'simplejson'
    ]
    json_impls = {}
    for libname in known_modnames:
        try:
            module = importlib.import_module(libname)
        except ImportError:
            pass
        else:
            json_impls[libname] = {
                'module': module,
                'version': module.__version__,
            }
    return json_impls


def benchmark_json_dumps():
    # TODO: remove this hack
    sys.path.append(ub.expandpath('~/code/ultrajson/tests'))
    from benchmarker import Benchmarker

    json_impls = available_json_impls()
    data_lut = json_test_data_generators()

    list(data_lut.keys())

    # These are the parameters that we benchmark over
    basis = {
        "input": [
            'Array with doubles',
            'Array with UTF-8 strings',
            # 'Medium complex object',
            'Array with True values',
            'Array of Dict[str, int]',
            # 'Dict of List[Dict[str, int]]',
            # 'Complex object'
        ],
        "size": [1, 32, 256, 1024, 2048],
        "impl": list(json_impls.keys()),
    }

    # The Benchmarker class is a new experimental API around timerit to
    # abstract away the details of timing a process over a grid of parameters,
    # serializing the results, and aggregating results from disparate runs.
    benchmark = Benchmarker(
        name='bench_json_dumps',
        num=100,
        bestof=10,
        verbose=2,
        basis=basis,
    )

    # For each variation of your experiment, create a row.
    for params in benchmark.iter_params():
        # Make any modifications you need to compute input kwargs for each
        # method here.
        impl_info = json_impls[params["impl"]]
        method = impl_info['module'].dumps
        impl_version = impl_info['version']
        params["impl_version"] = impl_version
        data = data_lut[params["input"]](params["size"])
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
    metric_key = "time" if RECORD_ALL else "mean_time"

    from benchmarker import result_analysis
    results = benchmark.result.to_result_list()

    analysis = result_analysis.ResultAnalysis(
        results,
        metrics=[metric_key],
        params=['impl'],
        metric_objectives={
            'min_time': 'min',
            'mean_time': 'min',
            'time': 'min',
        })
    analysis.analysis()
    analysis.table

    param_group = ['impl', 'impl_version']
    analysis.abalate(param_group)
    # benchmark_analysis(rows, xlabel, group_labels, basis, RECORD_ALL)

    xlabel = "size"
    # Set these to empty lists if they are not used
    group_labels = {
        "col": ["input"],
        "hue": ["impl"],
        "size": [],
    }
    import kwplot
    kwplot.autompl()
    facet = analysis.plot(xlabel, metric_key, group_labels)
    for ax in facet.axes.ravel():
        ax.set_xscale('log')
        ax.set_yscale('log')
    print('facet = {!r}'.format(facet))
    kwplot.show_if_requested()


if __name__ == "__main__":
    """
    CommandLine:
        python ~/code/ultrajson/tests/benchmark3.py --show
    """
    benchmark_json_dumps()
