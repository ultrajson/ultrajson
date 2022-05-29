import ubelt as ub
import numpy as np

def __tabulate_issue():
    # MWE for tabulate issue
    # The decimals are not aligned when using "," in the floatfmt
    import tabulate
    data = [
        [13213.2, 3213254.23, 432432.231,],
        [432432., 432.3, 3.2]
    ]
    print(tabulate.tabulate(data, headers=['a', 'b'], floatfmt=',.02f'))
    print(tabulate.tabulate(data, headers=['a', 'b'], floatfmt='.02f'))


def __groupby_issue():
    # MWE of an issue with pandas groupby
    import pandas as pd
    data = pd.DataFrame([
        {'p1': 'a', 'p2': 1, 'p3': 0},
        {'p1': 'a', 'p2': 1, 'p3': 0},
        {'p1': 'a', 'p2': 2, 'p3': 0},
        {'p1': 'b', 'p2': 2, 'p3': 0},
        {'p1': 'b', 'p2': 1, 'p3': 0},
        {'p1': 'b', 'p2': 1, 'p3': 0},
        {'p1': 'b', 'p2': 1, 'p3': 0},
    ])

    by = 'p1'
    key = list(data.groupby(by))[0][0]
    result = {
        'by': by,
        'key': key,
        'type(key)': type(key)
    }
    print('result = {}'.format(ub.repr2(result, nl=1)))
    assert not ub.iterable(key), (
        '`by` is specified as a scalar, so getting `key` as a scalar makes sense')

    by = ['p1']
    key = list(data.groupby(by))[0][0]
    result = {
        'by': by,
        'key': key,
        'type(key)': type(key)
    }
    print('result = {}'.format(ub.repr2(result, nl=1)))
    assert not ub.iterable(key), (
        '`by` is specified as a list of scalars (with one element), but we '
        'still get `key` as a scalar. This does not make sense')

    by = ['p1', 'p2']
    key = list(data.groupby(by))[0][0]
    result = {
        'by': by,
        'key': key,
        'type(key)': type(key)
    }
    print('result = {}'.format(ub.repr2(result, nl=1)))
    assert ub.iterable(key), (
        '`by` is specified as a list of scalars (with multiple elements), '
        'and we still get `key` as a tuple of values. This makes sense')


def aggregate_stats(data, suffix='', group_keys=None):
    """
    Given columns interpreted as containing stats, aggregate those stats
    within each group. For each row, any non-group, non-stat column
    with consistent values across that columns in the group is kept as-is,
    otherwise the new column for that row is set to None.

    Args:
        data (DataFrame):
            a data frame with columns: 'mean', 'std', 'min', 'max', and 'nobs'
            (possibly with a suffix)

        suffix (str):
            if the nobs, std, mean, min, and max have a suffix, specify it

        group_keys (List[str]):
            pass

    Returns:
        DataFrame:
            New dataframe where grouped rows have been aggregated into a single
            row.

    Example:
        >>> import sys, ubelt
        >>> sys.path.append(ubelt.expandpath('~/code/ultrajson'))
        >>> from json_benchmarks.benchmarker.util_stats import *  # NOQA
        >>> import pandas as pd
        >>> data = pd.DataFrame([
        >>>     #
        >>>     {'mean': 8, 'std': 1, 'min': 0, 'max': 1, 'nobs': 2, 'p1': 'a', 'p2': 1},
        >>>     {'mean': 6, 'std': 2, 'min': 0, 'max': 1, 'nobs': 3, 'p1': 'a', 'p2': 1},
        >>>     {'mean': 7, 'std': 3, 'min': 0, 'max': 2, 'nobs': 5, 'p1': 'a', 'p2': 2},
        >>>     {'mean': 5, 'std': 4, 'min': 0, 'max': 3, 'nobs': 7, 'p1': 'a', 'p2': 1},
        >>>     #
        >>>     {'mean': 3, 'std': 1, 'min': 0, 'max': 20, 'nobs': 6, 'p1': 'b', 'p2': 1},
        >>>     {'mean': 0, 'std': 2, 'min': 0, 'max': 20, 'nobs': 26, 'p1': 'b', 'p2': 2},
        >>>     {'mean': 9, 'std': 3, 'min': 0, 'max': 20, 'nobs': 496, 'p1': 'b', 'p2': 1},
        >>>     #
        >>>     {'mean': 5, 'std': 0, 'min': 0, 'max': 1, 'nobs': 2, 'p1': 'c', 'p2': 2},
        >>>     {'mean': 5, 'std': 0, 'min': 0, 'max': 1, 'nobs': 7, 'p1': 'c', 'p2': 2},
        >>>     #
        >>>     {'mean': 5, 'std': 2, 'min': 0, 'max': 2, 'nobs': 7, 'p1': 'd', 'p2': 2},
        >>>     #
        >>>     {'mean': 5, 'std': 2, 'min': 0, 'max': 2, 'nobs': 7, 'p1': 'e', 'p2': 1},
        >>> ])
        >>> print(data)
        >>> new_data = aggregate_stats(data)
        >>> print(new_data)
        >>> new_data1 = aggregate_stats(data, group_keys=['p1'])
        >>> print(new_data1)
        >>> new_data2 = aggregate_stats(data, group_keys=['p2'])
        >>> print(new_data2)
    """
    import pandas as pd

    # Stats groupings
    raw_stats_cols = ["nobs", "std", "mean", "max", "min"]
    stats_cols = [c + suffix for c in raw_stats_cols]
    mapper = dict(zip(stats_cols, raw_stats_cols))
    unmapper = dict(zip(raw_stats_cols, stats_cols))
    non_stats_cols = list(ub.oset(data.columns) - stats_cols)
    if group_keys is None:
        group_keys = non_stats_cols
    non_group_keys = list(ub.oset(non_stats_cols) - group_keys)

    new_rows = []
    for group_vals, group in list(data.groupby(group_keys)):
        # hack, is this a pandas bug in 1.4.1? Is it fixed? (Not in 1.4.2)
        if isinstance(group_keys, list) and len(group_keys) == 1:
            # For some reason, when we specify group keys as a list of one
            # element, we get a squeezed value out
            group_vals = (group_vals,)
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


def stats_dict(data, suffix=""):
    stats = {
        "nobs" + suffix: len(data),
        "mean" + suffix: data.mean(),
        "std" + suffix: data.std(),
        "min" + suffix: data.min(),
        "max" + suffix: data.max(),
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
        "nobs": np.array([s["nobs"] for s in stats]),
        "mean": np.array([s["mean"] for s in stats]),
        "std": np.array([s["std"] for s in stats]),
        "min": np.array([s["min"] for s in stats]),
        "max": np.array([s["max"] for s in stats]),
    }
    combine_stats_arrs(data)


def combine_stats_arrs(data):
    sizes = data["nobs"]
    means = data["mean"]
    stds = data["std"]
    mins = data["min"]
    maxs = data["max"]
    varis = stds * stds

    # TODO: ddof
    # https://github.com/Erotemic/misc/blob/28cf797b9b0f8bd82e3ebee2f6d0a688ecee2838/learn/stats.py#L128

    combo_size = sizes.sum()
    combo_mean = (sizes * means).sum() / combo_size

    mean_deltas = means - combo_mean

    sv = (sizes * varis).sum()
    sm = (sizes * (mean_deltas * mean_deltas)).sum()
    combo_vars = (sv + sm) / combo_size
    combo_std = np.sqrt(combo_vars)

    combo_stats = {
        "nobs": combo_size,
        "mean": combo_mean,
        "std": combo_std,
        "min": mins.min(),
        "max": maxs.max(),
    }
    return combo_stats
