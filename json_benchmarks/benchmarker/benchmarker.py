import json
import timerit
import ubelt as ub
import numpy as np
from dataclasses import dataclass
from json_benchmarks.benchmarker.process_context import ProcessContext


@dataclass
class BenchmarkerConfig:
    name    : str = None
    num     : int = 100
    bestof  : int = 10


class BenchmarkerResult:
    """
    Serialization for a single benchmark result
    """
    def __init__(self, context, rows):
        self.context = context
        self.rows = rows

    def __json__(self):
        data = {
            'type': 'benchmark_result',
            'context': self.context,
            'rows': self.rows,
        }
        return data

    @classmethod
    def from_json(cls, data):
        assert data['type'] == 'benchmark_result'
        self = cls(data['context'], data['rows'])
        return self

    @classmethod
    def load(cls, fpath):
        with open(fpath, 'r') as file:
            data = json.load(file)
        self = cls.from_json(data)
        return self

    def to_result_list(self):
        """
        Returns a list of result objects suitable for ResultAnalysis

        Returns:
            List[Result]
        """
        from json_benchmarks.benchmarker import result_analysis
        results = []
        for row in self.rows:
            result = result_analysis.Result(
                name=row['name'],
                metrics=row['metrics'],
                params=row['params'].copy(),
            )
            machine = self.context['machine']
            assert not ub.dict_isect(result.params, machine)
            result.params.update(machine)
            results.append(result)
        return results


class Benchmarker:
    """
    Helper to organize the execution and serialization of a benchmark

    Example:
        >>> import numpy as np
        >>> impl_lut = {
        >>>     'numpy': np.sum,
        >>>     'builtin': sum,
        >>> }
        >>> def data_lut(params):
        >>>     item = 42 if params['dtype'] == 'int' else 42.0
        >>>     data = [item] * params['size']
        >>>     return data
        >>> basis = {
        >>>     'impl': ['builtin', 'numpy'],
        >>>     'size': [10, 10000],
        >>>     'dtype': ['int', 'float'],
        >>> }
        >>> self = Benchmarker(name='demo', num=10, bestof=3, basis=basis)
        >>> for params in self.iter_params():
        >>>     impl = impl_lut[params['impl']]
        >>>     data = data_lut(params)
        >>>     for timer in self.measure():
        >>>         with timer:
        >>>             impl(data)
        >>> print('self.result = {}'.format(ub.repr2(self.result.__json__(), sort=0, nl=2, precision=8)))
        >>> dpath = ub.Path.appdir('benchmarker/demo').ensuredir()
        >>> self.dump_in_dpath(dpath)
    """
    def __init__(self, basis={}, verbose=1, **kwargs):
        self.basis = basis

        self.config = BenchmarkerConfig(**kwargs)

        self.ti = timerit.Timerit(
            num=self.config.num,
            bestof=self.config.bestof,
            verbose=verbose,
        )
        self.context = ProcessContext(name=self.config.name)
        self.rows = []
        self.RECORD_ALL = 0
        self.result = None

    def dump_in_dpath(self, dpath):
        dpath = ub.Path(dpath)
        timestamp = self.context.obj['stop_timestamp']
        fname = f'benchmarks_{self.config.name}_{timestamp}.json'
        fpath = dpath / fname

        with open(fpath, 'w') as file:
            json.dump(self.result.__json__(), file)
        return fpath

    def iter_params(self):
        self.context.start()
        grid_iter = list(ub.named_product(self.basis))
        for params in grid_iter:
            self.params = params
            self.key = ub.repr2(params, compact=1, si=1)
            yield params
        obj = self.context.stop()
        self.result = BenchmarkerResult(obj, self.rows)

    def measure(self):
        for timer in self.ti.reset(self.key):
            yield timer

        rows = self.rows
        ti = self.ti
        key = self.key
        params = self.params
        times = ti.robust_times()
        if self.RECORD_ALL:
            for time in times:
                metrics = {
                    "time": time,
                }
                row = {
                    'name': key,
                    'metrics': metrics,
                    'params': params,
                }
                rows.append(row)
        else:
            times = np.array(ti.robust_times())
            metrics = stats_dict(times, '_time')
            row = {
                'metrics': metrics,
                'params': params,
                'name': key,
            }
            rows.append(row)


def stats_dict(data, suffix=''):
    stats = {
        'nobs' + suffix: len(data),
        'mean' + suffix: data.mean(),
        'std' + suffix: data.std(),
        'min' + suffix: data.min(),
        'max' + suffix: data.max(),
    }
    return stats


def combine_stats(s1, s2):
    """
    Helper for combining mean and standard deviation of multiple measurements

    Args:
        s1 (dict): stats dict containing mean, std, and n
        s2 (dict): stats dict containing mean, std, and n

    Example:
        >>> basis = {
        >>>     'nobs1': [1, 10, 100, 10000],
        >>>     'nobs2': [1, 10, 100, 10000],
        >>> }
        >>> for params in ub.named_product(basis):
        >>>     data1 = np.random.rand(params['nobs1'])
        >>>     data2 = np.random.rand(params['nobs2'])
        >>>     data3 = np.hstack([data1, data2])
        >>>     s1 = stats_dict(data1)
        >>>     s2 = stats_dict(data2)
        >>>     s3 = stats_dict(data3)
        >>>     # Check that our combo works
        >>>     combo_s3 = combine_stats(s1, s2)
        >>>     compare = pd.DataFrame({'raw': s3, 'combo': combo_s3})
        >>>     print(compare)
        >>>     assert np.allclose(compare.raw, compare.combo)

    References:
        https://stackoverflow.com/questions/7753002/adding-combining-standard-deviations
        https://math.stackexchange.com/questions/2971315/how-do-i-combine-standard-deviations-of-two-groups
    """
    stats = [s1, s2]
    data = {
        'nobs': np.array([s['nobs'] for s in stats]),
        'mean': np.array([s['mean'] for s in stats]),
        'std': np.array([s['std'] for s in stats]),
        'min': np.array([s['min'] for s in stats]),
        'max': np.array([s['max'] for s in stats]),
    }
    combine_stats_arrs(data)


def combine_stats_arrs(data):
    sizes = data['nobs']
    means = data['mean']
    stds = data['std']
    mins = data['min']
    maxs = data['max']
    varis = stds * stds

    combo_size = sizes.sum()
    combo_mean = (sizes * means).sum() / combo_size

    mean_deltas = (means - combo_mean)

    sv = (sizes * varis).sum()
    sm = (sizes * (mean_deltas * mean_deltas)).sum()
    combo_vars = (sv + sm) / combo_size
    combo_std = np.sqrt(combo_vars)

    combo_stats = {
        'nobs': combo_size,
        'mean': combo_mean,
        'std': combo_std,
        'min': mins.min(),
        'max': maxs.max(),
    }
    return combo_stats
