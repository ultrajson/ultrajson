import json
from dataclasses import dataclass

import numpy as np
import timerit
import ubelt as ub

from json_benchmarks.benchmarker.process_context import ProcessContext


@dataclass
class BenchmarkerConfig:
    name: str = None
    num: int = 100
    bestof: int = 10


class BenchmarkerResult:
    """
    Serialization for a single benchmark result
    """

    def __init__(self, context, rows):
        self.context = context
        self.rows = rows

    def __json__(self):
        data = {
            "type": "benchmark_result",
            "context": self.context,
            "rows": self.rows,
        }
        return data

    @classmethod
    def from_json(cls, data):
        assert data["type"] == "benchmark_result"
        self = cls(data["context"], data["rows"])
        return self

    @classmethod
    def load(cls, fpath):
        with open(fpath) as file:
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
                name=row["name"],
                metrics=row["metrics"],
                params=row["params"].copy(),
            )
            machine = self.context["machine"]
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
        timestamp = self.context.obj["stop_timestamp"]
        fname = f"benchmarks_{self.config.name}_{timestamp}.json"
        fpath = dpath / fname

        with open(fpath, "w") as file:
            json.dump(self.result.__json__(), file)
        return fpath

    def iter_params(self):
        self.context.start()
        if isinstance(self.basis, dict):
            grid_iter = ub.named_product(self.basis)
        else:
            grid_iter = ub.flatten([ub.named_product(b) for b in self.basis])

        for params in grid_iter:
            self.params = params
            self.key = ub.repr2(params, compact=1, si=1)
            yield params
        obj = self.context.stop()
        self.result = BenchmarkerResult(obj, self.rows)

    def measure(self):
        yield from self.ti.reset(self.key)

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
                    "name": key,
                    "metrics": metrics,
                    "params": params,
                }
                rows.append(row)
        else:
            from json_benchmarks.benchmarker import util_stats

            times = np.array(ti.robust_times())
            metrics = util_stats.stats_dict(times, "_time")
            row = {
                "metrics": metrics,
                "params": params,
                "name": key,
            }
            rows.append(row)


def _test_demo():
    from json_benchmarks.benchmarker import BenchmarkerResult, result_analysis
    from json_benchmarks.benchmarker.benchmarker import Benchmarker
    import numpy as np

    impl_lut = {
        "numpy": np.sum,
        "builtin": sum,
    }

    def data_lut(params):
        item = 42 if params["dtype"] == "int" else 42.0
        data = [item] * params["size"]
        return data

    basis = {
        "impl": ["builtin", "numpy"],
        "size": [10, 10000],
        "dtype": ["int", "float"],
    }

    dpath = ub.Path.appdir("benchmarker/agg_demo").delete().ensuredir()

    def run_one_benchmark():
        self = Benchmarker(name="agg_demo", num=10, bestof=3, basis=basis)
        for params in self.iter_params():
            impl = impl_lut[params["impl"]]
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

    results = []
    for fpath in fpaths:
        data = json.loads(fpath.read_text())
        for row in data["rows"]:
            result = BenchmarkerResult.load(fpath)
            results.extend(result.to_result_list())

    analysis = result_analysis.ResultAnalysis(
        results,
        metrics=["min", "mean"],
        params=["impl"],
        metric_objectives={
            "min": "min",
            "mean": "min",
        },
    )
    analysis.analysis()
    # single_df = pd.DataFrame(data['rows'])
    # context = data['context']
    # single_df
