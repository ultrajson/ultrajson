"""
The analysis of the measurements
"""
import scriptconfig as scfg
import ubelt as ub


class AnalysisConfig(scfg.Config):
    default = {
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


def analyze_results(result_fpaths):
    import json

    from json_benchmarks import benchmarker
    from json_benchmarks.benchmarker import util_stats

    results = []
    for fpath in ub.ProgIter(result_fpaths, desc="load results"):
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
    stats_table = util_stats.aggregate_stats(table, suffix="_time", group_keys=["name"])

    single_size = stats_table[
        (stats_table["size"] == 256) | stats_table["size"].isnull()
    ]
    # single_size_combo = aggregate_stats(single_size, None)
    single_size_combo = util_stats.aggregate_stats(
        single_size, suffix="_time", group_keys=["name"]
    )

    param_group = ["impl", "impl_version"]
    single_size_combo["calls/sec"] = 1 / single_size_combo["mean_time"]
    # _single_size_combo = single_size_combo.copy()
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
    self = analysis  # NOQA

    data = stats_table
    plots = analysis.plot(
        xlabel,
        metric_key,
        group_labels,
        xscale="log",
        yscale="log",
        data=data,
    )
    plots
    kwplot.show_if_requested()
