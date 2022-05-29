import itertools as it
import math
import warnings

import numpy as np
import pandas as pd
import scipy
import scipy.stats  # NOQA
import ubelt as ub

# a list of common objectives
DEFAULT_METRIC_TO_OBJECTIVE = {
    "time": "min",
    "ap": "max",
    "acc": "max",
    "f1": "max",
    "mcc": "max",
    #
    "loss": "min",
    "brier": "min",
}


class Result(ub.NiceRepr):
    """
    Storage of names, parameters, and quality metrics for a single experiment.

    Attributes:
        name (str | None):
            Name of the experiment. Optional. This is unused in the analysis.
            (i.e. names will never be used computationally. Use them for keys)

        params (Dict[str, object]): configuration of the experiment.
            This is a dictionary mapping a parameter name to its value.

        metrics (Dict[str, float]): quantitative results of the experiment
            This is a dictionary for each quality metric computed on this
            result.

        meta (Dict | None): any other metadata about this result.
            This is unused in the analysis.

    Example:
        >>> self = Result.demo(rng=32)
        >>> print('self = {}'.format(self))
        self = <Result(name=53f57161,f1=0.33,acc=0.75,param1=1,param2=6.67,param3=a)>

    Example:
        >>> self = Result.demo(mode='alt', rng=32)
        >>> print('self = {}'.format(self))
    """

    def __init__(self, name, params, metrics, meta=None):
        self.name = name
        self.params = params
        self.metrics = metrics
        self.meta = meta

    def to_dict(self):
        row = ub.dict_union({"name": self.name}, self.metrics, self.params)
        return row

    def __nice__(self):
        row = self.to_dict()
        text = ub.repr2(row, compact=True, precision=2, sort=0)
        return text

    @classmethod
    def demo(cls, mode="null", rng=None):
        import string

        import kwarray
        import numpy as np

        rng = kwarray.ensure_rng(rng)

        if mode == "null":
            # The null hypothesis should generally be true here,
            # there is no relation between the results and parameters
            demo_param_space = {
                "param1": list(range(3)),
                "param2": np.linspace(0, 10, 10),
                "param3": list(string.ascii_lowercase[0:3]),
            }
            params = {k: rng.choice(b) for k, b in demo_param_space.items()}
            metrics = {
                "f1": rng.rand(),
                "acc": rng.rand(),
            }
        elif mode == "alt":
            # The alternative hypothesis should be true here, there is a
            # relationship between results two of the params.
            from scipy.special import expit

            params = {
                "u": rng.randint(0, 1 + 1),
                "v": rng.randint(-1, 1 + 1),
                "x": rng.randint(-2, 3 + 1),
                "y": rng.randint(-1, 2 + 1),
                "z": rng.randint(-0, 3 + 1),
            }
            noise = np.random.randn() * 1
            r = 3 * params["x"] + params["y"] ** 2 + 0.3 * params["z"] ** 3
            acc = expit(r / 20 + noise)
            metrics = {
                "acc": acc,
            }
        else:
            raise KeyError(mode)
        name = ub.hash_data(params)[0:8]
        self = cls(name, params, metrics)
        return self


class ResultAnalysis(ub.NiceRepr):
    """
    Groups and runs stats on results

    Runs statistical tests on sets of configuration-metrics pairs

    Attributes:
        results (List[Result]): list of results

        ignore_metrics (Set[str]): metrics to ignore

        ignore_params (Set[str]): parameters to ignore

        metric_objectives (Dict[str, str]):
            indicate if each metrix should be maximized "max" or minimized
            "min"

        metrics (List[str]):
            only consider these metrics

        params (List[str]):
            if given, only consider these params

        abalation_orders (Set[int]):
            The number of parameters to be held constant in each statistical
            grouping. Defaults to 1, so it groups together results where 1
            variable is held constant. Including 2 will include pairwise
            settings of parameters to be held constant. Using -1 or -2 means
            all but 1 or 2 parameters will be held constant, repsectively.

        default_objective (str):
            assume max or min for unknown metrics

    Example:
        >>> self = ResultAnalysis.demo()
        >>> self.analysis()

    Example:
        >>> self = ResultAnalysis.demo(num=5000, mode='alt')
        >>> self.analysis()

    Example:
        >>> # Given a list of experiments, configs, and results
        >>> # Create a ResultAnalysis object
        >>> results = ResultAnalysis([
        >>>     Result('expt0', {'param1': 2, 'param3': 'b'}, {'f1': 0.75}),
        >>>     Result('expt1', {'param1': 0, 'param3': 'c'}, {'f1': 0.92}),
        >>>     Result('expt2', {'param1': 1, 'param3': 'b'}, {'f1': 0.77}),
        >>>     Result('expt3', {'param1': 1, 'param3': 'a'}, {'f1': 0.67}),
        >>>     Result('expt4', {'param1': 0, 'param3': 'c'}, {'f1': 0.98}),
        >>>     Result('expt5', {'param1': 2, 'param3': 'a'}, {'f1': 0.86}),
        >>>     Result('expt6', {'param1': 1, 'param3': 'c'}, {'f1': 0.77}),
        >>>     Result('expt7', {'param1': 1, 'param3': 'c'}, {'f1': 0.41}),
        >>>     Result('expt8', {'param1': 1, 'param3': 'a'}, {'f1': 0.64}),
        >>>     Result('expt9', {'param1': 0, 'param3': 'b'}, {'f1': 0.95}),
        >>> ])
        >>> # Calling the analysis method prints something like the following
        >>> results.analysis()

        PARAMETER 'param1' - f1
        =======================
        f1       mean       std   max   min  num  best
        param1
        0       0.950  0.030000  0.98  0.92  3.0  0.98
        2       0.805  0.077782  0.86  0.75  2.0  0.86
        1       0.652  0.147377  0.77  0.41  5.0  0.77

        ANOVA hypothesis (roughly): the param 'param1' has no effect on the metric
            Reject this hypothesis if the p value is less than a threshold
          Rank-ANOVA: p=0.0397
          Mean-ANOVA: p=0.0277

        Pairwise T-Tests
          Is param1=0 about as good as param1=2?
            ttest_ind:  p=0.2058
          Is param1=1 about as good as param1=2?
            ttest_ind:  p=0.1508


        PARAMETER 'param3' - f1
        =======================
        f1          mean       std   max   min  num  best
        param3
        c       0.770000  0.255734  0.98  0.41  4.0  0.98
        b       0.823333  0.110151  0.95  0.75  3.0  0.95
        a       0.723333  0.119304  0.86  0.64  3.0  0.86

        ANOVA hypothesis (roughly): the param 'param3' has no effect on the metric
            Reject this hypothesis if the p value is less than a threshold
          Rank-ANOVA: p=0.5890
          Mean-ANOVA: p=0.8145

        Pairwise T-Tests
          Is param3=b about as good as param3=c?
            ttest_ind:  p=0.7266
          Is param3=a about as good as param3=b?
            ttest_ind:  p=0.3466
            ttest_rel:  p=0.3466
          Is param3=a about as good as param3=c?
            ttest_ind:  p=0.7626
    """

    def __init__(
        self,
        results,
        metrics=None,
        params=None,
        ignore_params=None,
        ignore_metrics=None,
        metric_objectives=None,
        abalation_orders={1},
        default_objective="max",
        p_threshold=0.05,
    ):
        self.results = results
        if ignore_metrics is None:
            ignore_metrics = set()
        if ignore_params is None:
            ignore_params = set()
        self.ignore_params = ignore_params
        self.ignore_metrics = ignore_metrics

        self.abalation_orders = abalation_orders
        self.default_objective = default_objective

        # encode if we want to maximize or minimize a metric
        if metric_objectives is None:
            metric_objectives = {}
        self.metric_objectives = DEFAULT_METRIC_TO_OBJECTIVE.copy()
        self.metric_objectives.update(metric_objectives)

        self.params = params
        self.metrics = metrics
        self.statistics = None
        self.p_threshold = p_threshold

        self._description = {}
        self._description["built"] = False
        self._description["num_results"] = len(self.results)

    def __nice__(self):
        return ub.repr2(self._description, si=1, sv=1)

    @classmethod
    def demo(cls, num=10, mode="null", rng=None):
        import kwarray

        rng = kwarray.ensure_rng(rng)
        results = [Result.demo(mode=mode, rng=rng) for _ in range(num)]
        if mode == "null":
            self = cls(results, metrics={"f1", "acc"})
        else:
            self = cls(results, metrics={"acc"})
        return self

    def run(self):
        self.build()
        self.report()

    def analysis(self):
        # alias for run
        return self.run()
        self.build()
        self.report()

    @ub.memoize_property
    def table(self):
        rows = [r.to_dict() for r in self.results]
        table = pd.DataFrame(rows)
        return table

    def metric_table(self):
        rows = [r.to_dict() for r in self.results]
        table = pd.DataFrame(rows)
        return table

    @ub.memoize_property
    def varied(self):
        config_rows = [r.params for r in self.results]
        sentinel = object()
        # pd.DataFrame(config_rows).channels
        varied = dict(ub.varied_values(config_rows, default=sentinel, min_variations=1))
        # remove nans
        varied = {
            k: {v for v in vs if not (isinstance(v, float) and math.isnan(v))}
            for k, vs in varied.items()
        }
        varied = {k: vs for k, vs in varied.items() if len(vs)}
        return varied

    def abalation_groups(self, param_group, k=2):
        """
        Return groups where the specified parameter(s) are varied, but all
        other non-ignored parameters are held the same.

        Args:
            param_group (str | List[str]):
                One or more parameters that are allowed to vary

            k (int):
                minimum number of items a group must contain to be returned

        Returns:
            List[DataFrame]:
                a list of subsets of in the table where all but the specified
                (non-ignored) parameters are allowed to vary.

        Example:
            >>> self = ResultAnalysis.demo()
            >>> param = 'param2'
            >>> self.abalation_groups(param)
        """
        if not ub.iterable(param_group):
            param_group = [param_group]
        table = self.table
        config_rows = [r.params for r in self.results]
        config_keys = list(map(set, config_rows))
        # if self.params:
        #     config_keys = list(self.params)
        if self.ignore_params:
            config_keys = [c - self.ignore_params for c in config_keys]
        isect_params = set.intersection(*config_keys)
        other_params = sorted(isect_params - set(param_group))
        groups = []
        for key, group in table.groupby(other_params, dropna=False):
            if len(group) >= k:
                groups.append(group)
        return groups

    def _objective_is_ascending(self, metric_key):
        """
        Args:
            metric_key (str): the metric in question

        Returns:
            bool:
                True if we should minimize the objective (lower is better)
                False if we should maximize the objective (higher is better)
        """
        objective = self.metric_objectives.get(metric_key, None)
        if objective is None:
            warnings.warn(f"warning assume {self.default_objective} for {metric_key=}")
            objective = self.default_objective
        ascending = objective == "min"
        return ascending

    def abalate(self, param_group):
        """
        TODO:
            rectify with test-group

        Example:
            >>> self = ResultAnalysis.demo(100)
            >>> param = 'param2'
            >>> # xdoctest: +REQUIRES(module:openskill)
            >>> self.abalate(param)

            >>> self = ResultAnalysis.demo()
            >>> param_group = ['param2', 'param3']
            >>> # xdoctest: +REQUIRES(module:openskill)
            >>> self.abalate(param_group)
        """
        if self.table is None:
            self.table = self.build_table()
        if not ub.iterable(param_group):
            param_group = [param_group]

        # For hashable generic dictionary
        from collections import namedtuple

        gd = namedtuple("config", param_group)

        # from types import SimpleNamespace
        param_unique_vals_ = (
            self.table[param_group].drop_duplicates().to_dict("records")
        )
        param_unique_vals = [gd(**d) for d in param_unique_vals_]
        # param_unique_vals = {p: self.table[p].unique().tolist() for p in param_group}
        score_improvements = ub.ddict(list)
        scored_obs = []
        skillboard = SkillTracker(param_unique_vals)
        groups = self.abalation_groups(param_group, k=2)

        for group in groups:
            for metric_key in self.metrics:
                ascending = self._objective_is_ascending(metric_key)

                group = group.sort_values(metric_key, ascending=ascending)
                subgroups = group.groupby(param_group)
                if ascending:
                    best_idx = subgroups[metric_key].idxmax()
                else:
                    best_idx = subgroups[metric_key].idxmin()
                best_group = group.loc[best_idx]
                best_group = best_group.sort_values(metric_key, ascending=ascending)

                for x1, x2 in it.product(best_group.index, best_group.index):
                    if x1 != x2:
                        r1 = best_group.loc[x1]
                        r2 = best_group.loc[x2]
                        k1 = gd(**r1[param_group])
                        k2 = gd(**r2[param_group])
                        diff = r1[metric_key] - r2[metric_key]
                        score_improvements[(k1, k2, metric_key)].append(diff)

                # metric_vals = best_group[metric_key].values
                # diffs = metric_vals[None, :] - metric_vals[:, None]
                best_group.set_index(param_group)
                # best_group[param_group]
                # best_group[metric_key].diff()
                scored_ranking = best_group[param_group + [metric_key]].reset_index(
                    drop=True
                )
                scored_obs.append(scored_ranking)
                ranking = [
                    gd(**d) for d in scored_ranking[param_group].to_dict("records")
                ]
                skillboard.observe(ranking)

        print(
            "skillboard.ratings = {}".format(
                ub.repr2(skillboard.ratings, nl=1, align=":")
            )
        )
        win_probs = skillboard.predict_win()
        print(f"win_probs = {ub.repr2(win_probs, nl=1)}")
        for key, improves in score_improvements.items():
            k1, k2, metric_key = key
            improves = np.array(improves)
            pos_delta = improves[improves > 0]
            print(
                f"\nWhen {k1} is better than {k2}, the improvement in {metric_key} is"
            )
            print(pd.DataFrame([pd.Series(pos_delta).describe().T]))
        return scored_obs

    def test_group(self, param_group, metric_key):
        """
        Get stats for a particular metric / constant group

        Args:
            param_group (List[str]): group of parameters to hold constant.
            metric_key (str): The metric to test.

        Returns:
            dict
            # TODO : document these stats clearly and accurately

        Example:
            >>> self = ResultAnalysis.demo(num=100)
            >>> print(self.table)
            >>> param_group = ['param2', 'param1']
            >>> metric_key = 'f1'
            >>> stats_row = self.test_group(param_group, metric_key)
            >>> print('stats_row = {}'.format(ub.repr2(stats_row, nl=2, sort=0, precision=2)))
        """
        param_group_name = ",".join(param_group)
        stats_row = {
            "param_name": param_group_name,
            "metric": metric_key,
        }
        # param_values = varied[param_name]
        # stats_row['param_values'] = param_values
        ascending = self._objective_is_ascending(metric_key)

        # Find all items with this particular param value
        value_to_metric_group = {}
        value_to_metric_stats = {}
        value_to_metric = {}

        varied_cols = sorted(self.varied.keys())

        # Not sure if this is the right name, these are the other param keys
        # that we are not directly investigating, but might have an impact.
        # We use these to select comparable rows for pairwise t-tests
        nuisance_cols = sorted(set(self.varied.keys()) - set(param_group))

        for param_value, group in self.table.groupby(param_group):
            metric_group = group[["name", metric_key] + varied_cols]
            metric_vals = metric_group[metric_key]
            metric_vals = metric_vals.dropna()
            if len(metric_vals) > 0:
                metric_stats = metric_vals.describe()
                value_to_metric_stats[param_value] = metric_stats
                value_to_metric_group[param_value] = metric_group
                value_to_metric[param_value] = metric_vals.values

        moments = pd.DataFrame(value_to_metric_stats).T
        moments = moments.sort_values("mean", ascending=ascending)
        moments.index.name = param_group_name
        moments.columns.name = metric_key
        ranking = moments["mean"].index.to_list()
        param_to_rank = ub.invert_dict(dict(enumerate(ranking)))

        # Determine a set of value pairs to do pairwise comparisons on
        value_pairs = ub.oset()
        # value_pairs.update(
        #     map(frozenset, ub.iter_window(moments.index, 2)))
        value_pairs.update(
            map(
                frozenset,
                ub.iter_window(
                    moments.sort_values("mean", ascending=ascending).index, 2
                ),
            )
        )

        # https://en.wikipedia.org/wiki/Kruskal%E2%80%93Wallis_one-way_analysis_of_variance
        # If the researcher can make the assumptions of an identically
        # shaped and scaled distribution for all groups, except for any
        # difference in medians, then the null hypothesis is that the
        # medians of all groups are equal, and the alternative
        # hypothesis is that at least one population median of one
        # group is different from the population median of at least one
        # other group.
        try:
            anova_krus_result = scipy.stats.kruskal(*value_to_metric.values())
        except ValueError:
            anova_krus_result = scipy.stats.stats.KruskalResult(np.nan, np.nan)

        # https://en.wikipedia.org/wiki/One-way_analysis_of_variance
        # The One-Way ANOVA tests the null hypothesis, which states
        # that samples in all groups are drawn from populations with
        # the same mean values
        if len(value_to_metric) > 1:
            anova_1way_result = scipy.stats.f_oneway(*value_to_metric.values())
        else:
            anova_1way_result = scipy.stats.stats.F_onewayResult(np.nan, np.nan)

        stats_row["anova_rank_H"] = anova_krus_result.statistic
        stats_row["anova_rank_p"] = anova_krus_result.pvalue
        stats_row["anova_mean_F"] = anova_1way_result.statistic
        stats_row["anova_mean_p"] = anova_1way_result.pvalue
        stats_row["moments"] = moments

        pair_stats_list = []
        for pair in value_pairs:
            pair_stats = {}
            param_val1, param_val2 = pair

            metric_vals1 = value_to_metric[param_val1]
            metric_vals2 = value_to_metric[param_val2]

            rank1 = param_to_rank[param_val1]
            rank2 = param_to_rank[param_val2]
            pair_stats["winner"] = param_val1 if rank1 < rank2 else param_val2
            pair_stats["value1"] = param_val1
            pair_stats["value2"] = param_val2
            pair_stats["n1"] = len(metric_vals1)
            pair_stats["n2"] = len(metric_vals2)

            TEST_ONLY_FOR_DIFFERENCE = True
            if TEST_ONLY_FOR_DIFFERENCE:
                if ascending:
                    # We want to minimize the metric
                    alternative = "less" if rank1 < rank2 else "greater"
                else:
                    # We want to maximize the metric
                    alternative = "greater" if rank1 < rank2 else "less"
            else:
                alternative = "two-sided"

            ind_kw = dict(
                equal_var=False,
                alternative=alternative,
            )
            ttest_ind_result = scipy.stats.ttest_ind(
                metric_vals1, metric_vals2, **ind_kw
            )

            if 0:
                from benchmarker.benchmarker import stats_dict

                stats1 = stats_dict(metric_vals1)
                stats2 = stats_dict(metric_vals2)
                scipy.stats.ttest_ind_from_stats(
                    stats1["mean"],
                    stats1["std"],
                    stats1["nobs"],
                    stats2["mean"],
                    stats2["std"],
                    stats2["nobs"],
                    **ind_kw,
                )
                # metric_vals1, metric_vals2, equal_var=False)

            scipy.stats.ttest_ind_from_stats

            pair_stats["ttest_ind"] = ttest_ind_result

            # Do relative checks, need to find comparable subgroups
            metric_group1 = value_to_metric_group[param_val1]
            metric_group2 = value_to_metric_group[param_val2]
            nuisance_vals1 = metric_group1[nuisance_cols]
            nuisance_vals2 = metric_group2[nuisance_cols]
            nk_to_group1 = dict(list(nuisance_vals1.groupby(nuisance_cols)))
            nk_to_group2 = dict(list(nuisance_vals2.groupby(nuisance_cols)))
            common = set(nk_to_group1) & set(nk_to_group2)
            comparable_indexes1 = []
            comparable_indexes2 = []
            if common:
                for nk in common:
                    group1 = nk_to_group1[nk]
                    group2 = nk_to_group2[nk]
                    # TODO: Not sure if taking the product of everything within
                    # the comparable group is correct or not. I think it is ok.
                    for i, j in it.product(group1.index, group2.index):
                        comparable_indexes1.append(i)
                        comparable_indexes2.append(j)

                comparable_groups1 = metric_group1.loc[comparable_indexes1, metric_key]
                comparable_groups2 = metric_group2.loc[comparable_indexes2, metric_key]

                # Does this need to have the values aligned?
                # I think that is the case giving my understanding of paired
                # t-tests, but the docs need a PR to make that more clear.
                ttest_rel_result = scipy.stats.ttest_rel(
                    comparable_groups1, comparable_groups2
                )
                pair_stats["n_common"] = len(common)
                pair_stats["ttest_rel"] = ttest_rel_result
            pair_stats_list.append(pair_stats)

        stats_row["pairwise"] = pair_stats_list
        return stats_row

    def build(self):
        import itertools as it

        if len(self.results) < 2:
            raise Exception("need at least 2 results")

        varied = self.varied.copy()
        if self.ignore_params:
            for k in self.ignore_params:
                varied.pop(k, None)
        if self.params:
            varied = ub.dict_isect(varied, self.params)

        # Experimental:
        # Find Auto-abalation groups
        # TODO: when the group size is -1, instead of showing all of the group
        # settings, for each group setting do the k=1 analysis within that group
        varied_param_names = list(varied.keys())
        num_varied_params = len(varied)
        held_constant_orders = {
            num_varied_params + i if i < 0 else i for i in self.abalation_orders
        }
        held_constant_orders = [i for i in held_constant_orders if i > 0]
        held_constant_groups = []
        for k in held_constant_orders:
            held_constant_groups.extend(
                list(map(list, it.combinations(varied_param_names, k)))
            )

        if self.metrics is None:
            avail_metrics = set.intersection(
                *[set(r.metrics.keys()) for r in self.results]
            )
            metrics_of_interest = sorted(avail_metrics - set(self.ignore_metrics))
        else:
            metrics_of_interest = self.metrics
        self.metrics_of_interest = metrics_of_interest
        self._description["metrics_of_interest"] = metrics_of_interest
        self._description["num_groups"] = len(held_constant_groups)

        # Analyze the impact of each parameter
        self.statistics = statistics = []
        for param_group in held_constant_groups:
            for metric_key in metrics_of_interest:
                stats_row = self.test_group(param_group, metric_key)
                statistics.append(stats_row)

        self.stats_table = pd.DataFrame(
            [
                ub.dict_diff(d, {"pairwise", "param_values", "moments"})
                for d in self.statistics
            ]
        )

        if len(self.stats_table):
            self.stats_table = self.stats_table.sort_values("anova_rank_p")

        self._description["built"] = True

    def report(self):
        stat_groups = ub.group_items(self.statistics, key=lambda x: x["param_name"])
        stat_groups_items = list(stat_groups.items())

        # Modify this order to change the grouping pattern
        grid = ub.named_product(
            {
                "stat_group_item": stat_groups_items,
                "metrics": self.metrics_of_interest,
            }
        )
        for grid_item in grid:
            self._report_one(grid_item)

        print(self.stats_table)

    def _report_one(self, grid_item):
        p_threshold = self.p_threshold
        metric_key = grid_item["metrics"]
        stat_groups_item = grid_item["stat_group_item"]

        param_name, stat_group = stat_groups_item
        stats_row = ub.group_items(stat_group, key=lambda x: x["metric"])[metric_key][0]
        title = f"PARAMETER: {param_name} - METRIC: {metric_key}"
        print("\n\n")
        print(title)
        print("=" * len(title))
        print(stats_row["moments"])
        anova_rank_p = stats_row["anova_rank_p"]
        anova_mean_p = stats_row["anova_mean_p"]
        # Rougly speaking
        print("")
        print(f"ANOVA: If p is low, the param {param_name!r} might have an effect")
        print(
            ub.color_text(
                f"  Rank-ANOVA: p={anova_rank_p:0.8f}",
                "green" if anova_rank_p < p_threshold else None,
            )
        )
        print(
            ub.color_text(
                f"  Mean-ANOVA: p={anova_mean_p:0.8f}",
                "green" if anova_mean_p < p_threshold else None,
            )
        )
        print("")
        print("Pairwise T-Tests")
        for pairstat in stats_row["pairwise"]:
            # Is this backwards?
            value1 = pairstat["value1"]
            value2 = pairstat["value2"]
            winner = pairstat["winner"]
            if value2 == winner:
                value1, value2 = value2, value1
            print(
                f"  If p is low, {param_name}={value1} may outperform {param_name}={value2}."
            )
            if "ttest_ind" in pairstat:
                ttest_ind_result = pairstat["ttest_ind"]
                print(
                    ub.color_text(
                        f"    ttest_ind:  p={ttest_ind_result.pvalue:0.8f}",
                        "green" if ttest_ind_result.pvalue < p_threshold else None,
                    )
                )
            if "ttest_rel" in pairstat:
                n_common = pairstat["n_common"]
                ttest_rel_result = pairstat["ttest_ind"]
                print(
                    ub.color_text(
                        f"    ttest_rel:  p={ttest_rel_result.pvalue:0.8f}, n_pairs={n_common}",
                        "green" if ttest_rel_result.pvalue < p_threshold else None,
                    )
                )

    def conclusions(self):
        conclusions = []
        for stat in self.statistics:
            param_name = stat["param_name"]
            metric = stat["metric"]
            for pairstat in stat["pairwise"]:
                value1 = pairstat["value1"]
                value2 = pairstat["value2"]
                winner = pairstat["winner"]
                if value2 == winner:
                    value1, value2 = value2, value1
                pvalue = stat = pairstat["ttest_ind"].pvalue
                txt = f"p={pvalue:0.8f}, If p is low, {param_name}={value1} may outperform {value2} on {metric}."
                conclusions.append(txt)
        return conclusions

    def plot(self, xlabel, metric_key, group_labels, **kwargs):
        """
        Args:
            group_labels (dict):
                Tells seaborn what attributes to use to distinsuish curves like
                hue, size, marker. Also can contain "col" for use with
                FacetGrid, and "fig" to separate different configurations into
                different figures.

        Returns:
            List[Dict]:
                A list for each figure containing info abou that figure for any
                postprocessing.

        Example:
            >>> self = ResultAnalysis.demo(num=1000, mode='alt')
            >>> self.analysis()
            >>> print('self = {}'.format(self))
            >>> print('self.varied = {}'.format(ub.repr2(self.varied, nl=1)))
            >>> # xdoctest: +REQUIRES(module:kwplot)
            >>> import kwplot
            >>> kwplot.autosns()
            >>> xlabel = 'x'
            >>> metric_key = 'acc'
            >>> group_labels = {
            >>>     'fig': ['u'],
            >>>     'col': ['y', 'v'],
            >>>     'hue': ['z'],
            >>>     'size': [],
            >>> }
            >>> kwargs = {'xscale': 'log', 'yscale': 'log'}
            >>> self.plot(xlabel, metric_key, group_labels, **kwargs)
        """
        import seaborn as sns

        sns.set()
        from matplotlib import pyplot as plt  # NOQA

        data = self.table
        data = data.sort_values(metric_key)
        for gname, labels in group_labels.items():
            if len(labels):
                new_col = []
                for row in data[labels].to_dict("records"):
                    item = ub.repr2(row, compact=1, si=1)
                    new_col.append(item)
                gkey = gname + "_key"
                data[gkey] = new_col

        plot_kws = {
            "x": xlabel,
            "y": metric_key,
        }
        for gname, labels in group_labels.items():
            if labels:
                plot_kws[gname] = gname + "_key"

        # Your variables may change
        # ax = plt.figure().gca()
        fig_params = plot_kws.pop("fig", [])

        facet_kws = {
            "sharex": True,
            "sharey": True,
        }
        # facet_kws['col'] = plot_kws.pop("col", None)
        # facet_kws['row'] = plot_kws.pop("row", None)
        # if not facet_kws['row']:
        #     facet_kws['col_wrap'] = 5
        plot_kws["row"] = plot_kws.get("row", None)
        # if not plot_kws['row']:
        #     plot_kws['col_wrap'] = 5

        if not fig_params:
            groups = [("", data)]
        else:
            groups = data.groupby(fig_params)

        if "marker" not in plot_kws:
            plot_kws["marker"] = "o"

        # We will want to overwrite this with our own std estimate
        plot_kws["ci"] = "sd"
        # err_style='band',
        # err_kws=None,

        # Use a consistent pallete across plots
        unique_hues = data["hue_key"].unique()
        palette = ub.dzip(unique_hues, sns.color_palette(n_colors=len(unique_hues)))
        plot_kws["palette"] = palette

        plots = []
        base_fnum = 1
        for fnum, (fig_key, group) in enumerate(groups, start=base_fnum):
            # TODO: seaborn doesn't give us any option to reuse an existing
            # figure or even specify what it's handle should be. A patch should
            # be submitted to add that feature, but in the meantime work around
            # it and use the figures they give us.

            # fig = plt.figure(fnum)
            # fig.clf()

            facet = sns.relplot(
                data=group,
                # kind='line',
                kind="scatter",
                facet_kws=facet_kws,
                **plot_kws,
            )

            fig = facet.figure
            fig.suptitle(fig_key)
            fig.tight_layout()
            # facet = sns.FacetGrid(group, **facet_kws)
            # facet.map_dataframe(sns.lineplot, x=xlabel, y=metric_key, **plot_kws)
            # facet.add_legend()

            plot = {
                "fig": fig,
                "facet": facet,
            }
            plots.append(plot)

        for plot in plots:
            xscale = kwargs.get("xscale", None)
            yscale = kwargs.get("yscale", None)
            for ax in plot["facet"].axes.ravel():
                if xscale is not None:
                    try:
                        ax.set_xscale(xscale)
                    except ValueError:
                        pass
                if yscale is not None:
                    try:
                        ax.set_yscale(yscale)
                    except ValueError:
                        pass
        return plots


class SkillTracker:
    """
    Wrapper around openskill

    Args:
        player_ids (List[T]):
            a list of ids (usually ints) used to represent each player

    Example:
        >>> # xdoctest: +REQUIRES(module:openskill)
        >>> self = SkillTracker([1, 2, 3, 4, 5])
        >>> self.observe([2, 3])  # Player 2 beat player 3.
        >>> self.observe([1, 2, 5, 3])  # Player 3 didnt play this round.
        >>> self.observe([2, 3, 4, 5, 1])  # Everyone played, player 2 won.
        >>> win_probs = self.predict_win()
        >>> print('win_probs = {}'.format(ub.repr2(win_probs, nl=1, precision=2)))
        win_probs = {
            1: 0.20,
            2: 0.21,
            3: 0.19,
            4: 0.20,
            5: 0.20,
        }

    Requirements:
        openskill
    """

    def __init__(self, player_ids):
        import openskill

        self.player_ids = player_ids
        self.ratings = {m: openskill.Rating() for m in player_ids}
        # self.observations = []

    def predict_win(self):
        """
        Estimate the probability that a particular player will win given the
        current ratings.

        Returns:
            Dict[T, float]: mapping from player ids to win probabilites
        """
        from openskill import predict_win

        teams = [[p] for p in list(self.ratings.keys())]
        ratings = [[r] for r in self.ratings.values()]
        probs = predict_win(ratings)
        win_probs = {team[0]: prob for team, prob in zip(teams, probs)}
        return win_probs

    def observe(self, ranking):
        """
        After simulating a round, pass the ranked order of who won
        (winner is first, looser is last) to this function. And it
        updates the rankings.

        Args:
            ranking (List[T]):
                ranking of all the players that played in this round
                winners are at the front (0-th place) of the list.
        """
        import openskill

        # self.observations.append(ranking)
        ratings = self.ratings
        team_standings = [[r] for r in ub.take(ratings, ranking)]
        # new_values = openskill.rate(team_standings)  # Not inplace
        # new_ratings = [openskill.Rating(*new[0]) for new in new_values]
        new_team_ratings = openskill.rate(team_standings)
        new_ratings = [new[0] for new in new_team_ratings]
        ratings.update(ub.dzip(ranking, new_ratings))
