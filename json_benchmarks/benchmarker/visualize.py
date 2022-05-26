import pandas as pd
import ubelt as ub


def benchmark_analysis(rows, xlabel, group_labels, basis, ):
    # xlabel = "size"
    # Set these to empty lists if they are not used
    # group_labels = {
    #     "col": ["input"],
    #     "hue": ["impl"],
    #     "size": [],
    # }
    # group_keys = {}
    # for gname, labels in group_labels.items():
    #     group_keys[gname + "_key"] = ub.repr2(
    #         ub.dict_isect(params, labels), compact=1, si=1
    #     )
    # key = ub.repr2(params, compact=1, si=1)

    from process_tracker.result_analysis import SkillTracker
    RECORD_ALL = 0

    USE_OPENSKILL = True

    RECORD_ALL = 0
    metric_key = "time" if RECORD_ALL else "min"

    # The rows define a long-form pandas data array.
    # Data in long-form makes it very easy to use seaborn.
    data = pd.DataFrame(rows)
    data = data.sort_values(metric_key)

    if RECORD_ALL:
        # Show the min / mean if we record all
        min_times = data.groupby("key").min().rename({"time": "min"}, axis=1)
        mean_times = (
            data.groupby("key")[["time"]].mean().rename({"time": "mean"}, axis=1)
        )
        stats_data = pd.concat([min_times, mean_times], axis=1)
        stats_data = stats_data.sort_values("min")
    else:
        stats_data = data

    if USE_OPENSKILL:
        # Track the "skill" of each method
        # The idea is that each setting of parameters is a game, and each
        # "impl" is a player. We rank the players by which is fastest, and
        # update their ranking according to the Weng-Lin Bayes ranking model.
        # This does not take the fact that some "games" (i.e.  parameter
        # settings) are more important than others, but it should be fairly
        # robust on average.
        skillboard = SkillTracker(basis["impl"])

    other_keys = sorted(
        set(stats_data.columns)
        - {"key", "impl", "min", "mean", "hue_key", "size_key", "style_key"}
    )
    for params, variants in stats_data.groupby(other_keys):
        variants = variants.sort_values("mean")
        ranking = variants["impl"].reset_index(drop=True)

        mean_speedup = variants["mean"].max() / variants["mean"]
        stats_data.loc[mean_speedup.index, "mean_speedup"] = mean_speedup
        min_speedup = variants["min"].max() / variants["min"]
        stats_data.loc[min_speedup.index, "min_speedup"] = min_speedup

        if USE_OPENSKILL:
            skillboard.observe(ranking)

    print("Statistics:")
    print(stats_data)

    if USE_OPENSKILL:
        win_probs = skillboard.predict_win()
        win_probs = ub.sorted_vals(win_probs, reverse=True)
        print(
            "Aggregated Rankings = {}".format(
                ub.repr2(win_probs, nl=1, precision=4, align=":")
            )
        )

    plot = True
    if plot:
        # import seaborn as sns
        # kwplot autosns works well for IPython and script execution.
        # not sure about notebooks.
        import seaborn as sns

        sns.set()
        from matplotlib import pyplot as plt

        plotkw = {}
        for gname, labels in group_labels.items():
            if labels:
                plotkw[gname] = gname + "_key"

        # Your variables may change
        # ax = plt.figure().gca()
        col = plotkw.pop("col")
        facet = sns.FacetGrid(data, col=col, sharex=False, sharey=False)
        facet.map_dataframe(sns.lineplot, x=xlabel, y=metric_key, marker="o", **plotkw)
        facet.add_legend()
        # sns.lineplot(data=data, )
        # ax.set_title('JSON Benchmarks')
        # ax.set_xlabel('Size')
        # ax.set_ylabel('Time')
        # ax.set_xscale('log')
        # ax.set_yscale('log')

        try:
            __IPYTHON__
        except NameError:
            plt.show()
