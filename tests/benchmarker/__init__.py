"""
A helper module for executing, serializing, combining, and comparing benchmarks
"""

__mkinit__ = """
# Autogenerate this file
mkinit ~/code/ultrajson/tests/benchmarker/__init__.py -w
"""

__version__ = '0.1.0'

from benchmarker import aggregate
from benchmarker import benchmarker
from benchmarker import process_context
from benchmarker import result_analysis
from benchmarker import util_json
from benchmarker import visualize

from benchmarker.aggregate import (demo, demo_data,)
from benchmarker.benchmarker import (Benchmarker, BenchmarkerConfig,
                                     BenchmarkerResult, combine_stats,
                                     stats_dict,)
from benchmarker.process_context import (ProcessContext,)
from benchmarker.result_analysis import (Result, ResultAnalysis, SkillTracker,)
from benchmarker.util_json import (ensure_json_serializable,
                                   find_json_unserializable,
                                   indexable_allclose,)
from benchmarker.visualize import (benchmark_analysis,)

__all__ = ['Benchmarker', 'BenchmarkerConfig', 'BenchmarkerResult',
           'ProcessContext', 'Result', 'ResultAnalysis', 'SkillTracker',
           'aggregate', 'benchmark_analysis', 'benchmarker', 'combine_stats',
           'demo', 'demo_data', 'ensure_json_serializable',
           'find_json_unserializable', 'indexable_allclose', 'process_context',
           'result_analysis', 'stats_dict', 'util_json', 'visualize']
