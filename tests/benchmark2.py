import sys
import random
import timerit
import ubelt as ub


def data_lut(input, size):
    if input == 'Array with UTF-8 strings':
        test_object = []
        for x in range(size):
            test_object.append(
                "نظام الحكم سلطاني وراثي "
                "في الذكور من ذرية السيد تركي بن سعيد بن سلطان ويشترط فيمن يختار لولاية"
                " الحكم من بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين "
            )
        return test_object
    elif input == 'Array with doubles':
        test_object = []
        for x in range(256):
            test_object.append(sys.maxsize * random.random())
    else:
        raise KeyError(input)


def benchmark_json_dumps():
    import pandas as pd
    import ujson
    import json

    JSON_IMPLS = {
        "ujson": ujson,  # Our json
        "json": json,  # Python's json
    }

    if True:
        import nujson
        JSON_IMPLS['nujson'] = nujson
        import orjson
        JSON_IMPLS['nujson'] = orjson
        import simplejson
        JSON_IMPLS['simplejson'] = simplejson

    def method_lut(impl):
        return JSON_IMPLS[impl].dumps

    # Change params here to modify number of trials
    ti = timerit.Timerit(10000, bestof=10, verbose=1)

    # if True, record every trail run and show variance in seaborn
    # if False, use the standard timerit min/mean measures
    RECORD_ALL = True
    USE_OPENSKILL = True

    # These are the parameters that we benchmark over
    basis = {
        'input': [
             'Array with UTF-8 strings',
             'Array with doubles',
        ],
        'size': [1, 32, 256, 1024, 2048],
        'impl': list(JSON_IMPLS.keys()),
    }
    xlabel = 'size'
    # Set these to empty lists if they are not used
    group_labels = {
        'col': ['input'],
        'hue': ['impl'],
        'size': [],
    }
    grid_iter = list(ub.named_product(basis))

    # For each variation of your experiment, create a row.
    rows = []
    for params in grid_iter:
        group_keys = {}
        for gname, labels in group_labels.items():
            group_keys[gname + '_key'] = ub.repr2(
                ub.dict_isect(params, labels), compact=1, si=1)
        key = ub.repr2(params, compact=1, si=1)
        # Make any modifications you need to compute input kwargs for each
        # method here.
        method = method_lut(params['impl'])
        data = data_lut(params['input'], params['size'])
        # Timerit will run some user-specified number of loops.
        # and compute time stats with similar methodology to timeit
        for timer in ti.reset(key):
            # Put any setup logic you dont want to time here.
            # ...
            with timer:
                # Put the logic you want to time here
                method(data)

        if RECORD_ALL:
            # Seaborn will show the variance if this is enabled, otherwise
            # use the robust timerit mean / min times
            # chunk_iter = ub.chunks(ti.times, ti.bestof)
            # times = list(map(min, chunk_iter))  # TODO: timerit method for this
            times = ti.robust_times()
            for time in times:
                row = {
                    'time': time,
                    'key': key,
                    **group_keys,
                    **params,
                }
                rows.append(row)
        else:
            row = {
                'mean': ti.mean(),
                'min': ti.min(),
                'key': key,
                **group_keys,
                **params,
            }
            rows.append(row)

    time_key = 'time' if RECORD_ALL else 'min'

    # The rows define a long-form pandas data array.
    # Data in long-form makes it very easy to use seaborn.
    data = pd.DataFrame(rows)
    data = data.sort_values(time_key)

    if RECORD_ALL:
        # Show the min / mean if we record all
        min_times = data.groupby('key').min().rename({'time': 'min'}, axis=1)
        mean_times = data.groupby('key')[['time']].mean().rename({'time': 'mean'}, axis=1)
        stats_data = pd.concat([min_times, mean_times], axis=1)
        stats_data = stats_data.sort_values('min')
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
        skillboard = SkillTracker(basis['impl'])

    other_keys = sorted(set(stats_data.columns) - {'key', 'impl', 'min', 'mean', 'hue_key', 'size_key', 'style_key'})
    for params, variants in stats_data.groupby(other_keys):
        variants = variants.sort_values('mean')
        ranking = variants['impl'].reset_index(drop=True)

        mean_speedup = variants['mean'].max() / variants['mean']
        stats_data.loc[mean_speedup.index, 'mean_speedup'] = mean_speedup
        min_speedup = variants['min'].max() / variants['min']
        stats_data.loc[min_speedup.index, 'min_speedup'] = min_speedup

        if USE_OPENSKILL:
            skillboard.observe(ranking)

    print('Statistics:')
    print(stats_data)

    if USE_OPENSKILL:
        win_probs = skillboard.predict_win()
        win_probs = ub.sorted_vals(win_probs, reverse=True)
        print('Aggregated Rankings = {}'.format(ub.repr2(
            win_probs, nl=1, precision=4, align=':')))

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
                plotkw[gname] = gname + '_key'

        # Your variables may change
        # ax = plt.figure().gca()
        col = plotkw.pop('col')
        facet = sns.FacetGrid(data, col=col, sharex=False, sharey=False)
        facet.map_dataframe(sns.lineplot, x=xlabel, y=time_key, marker='o',
                            **plotkw)
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


class SkillTracker:
    """
    Wrapper around openskill

    Args:
        player_ids (List[T]):
            a list of ids (usually ints) used to represent each player

    Example:
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
    """

    def __init__(self, player_ids):
        import openskill
        self.player_ids = player_ids
        self.ratings = {m: openskill.Rating() for m in player_ids}
        self.observations = []

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
        self.observations.append(ranking)
        ratings = self.ratings
        team_standings = [[r] for r in ub.take(ratings, ranking)]
        new_values = openskill.rate(team_standings)  # Not inplace
        new_ratings = [openskill.Rating(*new[0]) for new in new_values]
        ratings.update(ub.dzip(ranking, new_ratings))


if __name__ == '__main__':
    """
    CommandLine:
        python ~/code/ultrajson/tests/benchmark2.py
    """
    benchmark_json_dumps()
