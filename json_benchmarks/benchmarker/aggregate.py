import json
import pandas as pd
import ubelt as ub


def demo_data():
    from json_benchmarks.benchmarker.benchmarker import Benchmarker
    import numpy as np
    impl_lut = {
        'numpy': np.sum,
        'builtin': sum,
    }
    def data_lut(params):
        item = 42 if params['dtype'] == 'int' else 42.0
        data = [item] * params['size']
        return data
    basis = {
        'impl': ['builtin', 'numpy'],
        'size': [10, 10000],
        'dtype': ['int', 'float'],
    }

    dpath = ub.Path.appdir('benchmarker/agg_demo').delete().ensuredir()

    def run_one_benchmark():
        self = Benchmarker(name='agg_demo', num=10, bestof=3, basis=basis)
        for params in self.iter_params():
            impl = impl_lut[params['impl']]
            data = data_lut(params)
            for timer in self.measure():
                with timer:
                    impl(data)
        fpath = self.dump_in_dpath(dpath)
        return fpath

    # Run the benchmark multiple times
    fpaths = []
    for _ in range(5):
        fpath = run_one_benchmark()
        fpaths.append(fpath)

    return fpaths


def demo():
    from json_benchmarks.benchmarker import BenchmarkerResult
    from json_benchmarks.benchmarker import result_analysis
    fpaths = demo_data()

    results = []
    for fpath in fpaths:
        data = json.loads(fpath.read_text())
        for row in data['rows']:
            result = BenchmarkerResult.load(fpath)
            results.extend(result.to_result_list())

    analysis = result_analysis.ResultAnalysis(
        results,
        metrics=['min', 'mean'],
        params=['impl'],
        metric_objectives={
            'min': 'min',
            'mean': 'min',
        })
    analysis.analysis()
    # single_df = pd.DataFrame(data['rows'])
    # context = data['context']
    # single_df
