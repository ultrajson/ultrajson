"""
Main definition of the benchmarks
"""
import json
import ubelt as ub
import scriptconfig as scfg
from json_benchmarks import benchmarker
from json_benchmarks import datagen

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
        'disable': scfg.Value([], choices=KNOWN_LIBRARIES, help=ub.paragraph(
            '''
            Remove specified libraries from the benchmarks
            '''
        )),

        'factor': scfg.Value(1.0, help=ub.paragraph(
            '''
            Specify as a fraction to speed up benchmarks for development /
            testing
            ''')),

        'cache_dir': scfg.Value(None, help=ub.paragraph(
            '''
            Location for benchmark cache.
            Defaults to $XDG_CACHE/ujson/benchmark_results/
            ''')),
    }

    def normalize(self):
        dpath = self['cache_dir']
        if dpath is None:
            dpath = ub.Path.appdir('ujson/benchmark_results')
        dpath = ub.Path(dpath)
        self['cache_dir'] = dpath


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


def benchmark_json():
    json_impls = available_json_impls()

    data_lut = datagen.json_test_data_generators()

    # These are the parameters that we benchmark over
    common_basis = {
        "impl": list(json_impls.keys()),
        "func": ['dumps', 'loads'],
    }
    sized_basis = {
        "input": [
            'Array with doubles',
            'Array with UTF-8 strings',
            # 'Medium complex object',
            'Array with True values',
            'Array of Dict[str, int]',
            # 'Dict of List[Dict[str, int]]',
            # 'Complex object'
        ],
        "size": [1, 2, 4, 8, 16, 32, 128, 256, 512],
        # 1024, 2048, 4096, 8192, 12288],
    }
    predefined_basis = {
        "input": [
            'Complex object'
        ],
        'size': [None],
    }

    basis = [
        ub.dict_union(common_basis, predefined_basis),
        ub.dict_union(common_basis, sized_basis),
    ]

    # The Benchmarker class is a new experimental API around timerit to
    # abstract away the details of timing a process over a grid of parameters,
    # serializing the results, and aggregating results from disparate runs.
    benchmark = benchmarker.Benchmarker(
        name='bench_json',
        num=100,
        bestof=10,
        verbose=3,
        basis=basis,
    )

    def is_blocked(params):
        if params['input'] == 'Complex object' and params['impl'] == 'orjson':
            return True

    # For each variation of your experiment, create a row.
    for params in benchmark.iter_params():
        if is_blocked(params):
            continue
        # Make any modifications you need to compute input kwargs for each
        # method here.
        impl_info = json_impls[params["impl"]]
        params["impl_version"] = impl_info['version']
        module = impl_info['module']
        if params['func'] == 'dumps':
            method = module.dumps
            data = data_lut[params["input"]](params["size"])
        elif params['func'] == 'loads':
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

    dpath = ub.Path.appdir('ujson/benchmark_results').ensuredir()
    result_fpath = benchmark.dump_in_dpath(dpath)
    return result_fpath


def aggregate_results(result_fpaths):
    import json
    results = []
    for fpath in result_fpaths:
        data = json.loads(fpath.read_text())
        for row in data['rows']:
            result = benchmarker.BenchmarkerResult.load(fpath)
            results.extend(result.to_result_list())

    RECORD_ALL = 0
    metric_key = "time" if RECORD_ALL else "mean_time"

    # results = benchmark.result.to_result_list()

    analysis = benchmarker.result_analysis.ResultAnalysis(
        results,
        metrics=[metric_key],
        params=['impl'],
        metric_objectives={
            'min_time': 'min',
            'mean_time': 'min',
            'time': 'min',
        })
    analysis.analysis()

    table = analysis.table

    def aggregate_time_stats(data, group_keys=None):
        """
        Given columns interpreted as containing stats, aggregate those stats
        within each group. For each row, any non-group, non-stat column
        with consistent values across that columns in the group is kept as-is,
        otherwise the new column for that row is set to None.
        """
        import pandas as pd
        # Stats groupings
        stats_cols = [
            'nobs_time',
            'std_time',
            'mean_time',
            'max_time',
            'min_time',
        ]
        mapper = {c: c.replace('_time', '') for c in stats_cols}
        unmapper = ub.invert_dict(mapper)
        non_stats_cols = list(ub.oset(data.columns) - stats_cols)
        if group_keys is None:
            group_keys = non_stats_cols
        non_group_keys = list(ub.oset(non_stats_cols) - group_keys)
        from json_benchmarks.benchmarker.benchmarker import combine_stats_arrs
        new_rows = []
        for group_vals, group in list(data.groupby(group_keys)):
            # hack, is this a pandas bug in 1.4.1? Is it fixed
            if isinstance(group_keys, list) and not isinstance(group_vals, list):
                group_vals = [group_vals]
            stat_data = group[stats_cols].rename(mapper, axis=1)
            new_stats = combine_stats_arrs(stat_data)
            new_time_stats = ub.map_keys(unmapper, new_stats)
            new_row = ub.dzip(group_keys, group_vals)
            if non_group_keys:
                for k in non_group_keys:
                    unique_vals = group[k].unique()
                    if len(unique_vals) == 1:
                        new_row[k] = unique_vals[0]
                    else:
                        new_row[k] = None
            new_row.update(new_time_stats)
            new_rows.append(new_row)
        new_data = pd.DataFrame(new_rows)
        return new_data

    single_size = table[(table['size'] == 256) | table['size'].isnull()]
    # single_size_combo = aggregate_time_stats(single_size, None)
    single_size_combo = aggregate_time_stats(single_size, ['name'])

    param_group = ['impl', 'impl_version']
    single_size_combo['calls/sec'] = 1 / single_size_combo['mean_time']
    _single_size_combo = single_size_combo.copy()
    _single_size_combo['calls/sec'] = _single_size_combo['calls/sec'].apply(lambda x: '{:,.02f}'.format(x))
    piv = _single_size_combo.pivot(['input', 'func'], param_group, 'calls/sec')
    print('Table for size=256')
    print(piv)

    analysis.abalate(param_group)
    # benchmark_analysis(rows, xlabel, group_labels, basis, RECORD_ALL)

    xlabel = "size"
    # Set these to empty lists if they are not used
    group_labels = {
        "fig": ["input"],
        "col": ["func"],
        "hue": ["impl", "impl_version"],
        "size": [],
    }
    import kwplot
    kwplot.autosns()
    plots = analysis.plot(xlabel, metric_key, group_labels)
    for plot in plots:
        for ax in plot['facet'].axes.ravel():
            ax.set_xscale('log')
            ax.set_yscale('log')
    kwplot.show_if_requested()


def main():
    from json_benchmarks import core
    config = core.JSONBenchmarkConfig(cmdline=True)
    dpath = config['cache_dir']

    run = 1
    if run:
        result_fpath = core.benchmark_json()
        print('result_fpath = {!r}'.format(result_fpath))
        result_fpaths = [result_fpath]

    agg = 1
    if agg:
        result_fpaths = list(dpath.glob('benchmarks*.json'))

    core.aggregate_results(result_fpaths)
    # results_output_table(libraries)
