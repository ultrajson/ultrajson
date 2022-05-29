"""
A helper module for executing, serializing, combining, and comparing benchmarks
"""

__mkinit__ = """
# Autogenerate this file
mkinit ~/code/ultrajson/json_benchmarks/benchmarker/__init__.py -w
"""

__version__ = "0.1.0"

from json_benchmarks.benchmarker import benchmarker
from json_benchmarks.benchmarker import process_context
from json_benchmarks.benchmarker import result_analysis
from json_benchmarks.benchmarker import util_json
from json_benchmarks.benchmarker import util_stats
from json_benchmarks.benchmarker import visualize

from json_benchmarks.benchmarker.benchmarker import (Benchmarker,
                                                     BenchmarkerConfig,
                                                     BenchmarkerResult,)
from json_benchmarks.benchmarker.process_context import (ProcessContext,)
from json_benchmarks.benchmarker.result_analysis import (
    DEFAULT_METRIC_TO_OBJECTIVE, Result, ResultAnalysis, SkillTracker,)
from json_benchmarks.benchmarker.util_json import (ensure_json_serializable,
                                                   find_json_unserializable,
                                                   indexable_allclose,)
from json_benchmarks.benchmarker.util_stats import (aggregate_stats,
                                                    combine_stats,
                                                    combine_stats_arrs,
                                                    stats_dict,)
from json_benchmarks.benchmarker.visualize import (benchmark_analysis,)

__all__ = ['Benchmarker', 'BenchmarkerConfig', 'BenchmarkerResult',
           'DEFAULT_METRIC_TO_OBJECTIVE', 'ProcessContext', 'Result',
           'ResultAnalysis', 'SkillTracker', 'aggregate_stats',
           'benchmark_analysis', 'benchmarker', 'combine_stats',
           'combine_stats_arrs', 'ensure_json_serializable',
           'find_json_unserializable', 'indexable_allclose', 'process_context',
           'result_analysis', 'stats_dict', 'util_json', 'util_stats',
           'visualize']
