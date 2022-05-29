"""
Main definition of the benchmarks
"""
import scriptconfig as scfg
import ubelt as ub

from json_benchmarks import measures
from json_benchmarks import analysis


class CoreConfig(scfg.Config):
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
    config = CoreConfig(cmdline=cmdline, data=kwargs)
    dpath = config["cache_dir"]

    run = config["mode"] in {"all", "single", "run"}
    if run:
        result_fpath = measures.benchmark_json()
        print(f"result_fpath = {result_fpath!r}")
        result_fpaths = [result_fpath]

    agg = config["mode"] not in {"single"}
    if agg:
        result_fpaths = list(dpath.glob("benchmarks*.json"))

    analyze = config["mode"] in {"all", "single", "analyze"}
    if analyze:
        analysis.analyze_results(result_fpaths)
