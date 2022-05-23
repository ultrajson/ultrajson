
def check_ttest():
    import scipy
    import scipy.stats  # NOQA
    from benchmarker.benchmarker import stats_dict
    import numpy as np
    metric_vals1 = np.random.randn(10000) + 0.01
    metric_vals2 = np.random.randn(1000)

    stats1 = stats_dict(metric_vals1)
    stats2 = stats_dict(metric_vals2)

    ind_kw = dict(
        equal_var=0,
        # alternative='two-sided'
        alternative='less' if stats1['mean'] < stats2['mean'] else 'greater'
    )

    # Not sure why these are slightly different
    res1 = scipy.stats.ttest_ind(metric_vals1, metric_vals2, **ind_kw)

    res2 = scipy.stats.ttest_ind_from_stats(
        stats1['mean'], stats1['std'], stats1['n'],
        stats2['mean'], stats2['std'], stats2['n'],
        **ind_kw
    )
    print('res1 = {!r}'.format(res1))
    print('res2 = {!r}'.format(res2))
