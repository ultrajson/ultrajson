import random
import sys

import ubelt as ub


def json_test_data_generators():
    """
    Generates data for benchmarks with various sizes

    Returns:
        Dict[str, callable]:
            a mapping from test data name to its generator

    Example:
        >>> data_lut = json_test_data_generators()
        >>> size = 2
        >>> keys = sorted(set(data_lut) - {'Complex object'})
        >>> for key in keys:
        >>>     func = data_lut[key]
        >>>     test_object = func(size)
        >>>     print('key = {!r}'.format(key))
        >>>     print('test_object = {!r}'.format(test_object))
    """
    data_lut = {}

    def _register_data(name):
        def _wrap(func):
            data_lut[name] = func

        return _wrap

    # seed if desired
    # rng = random.Random(0)
    rng = random

    @_register_data("Array with doubles")
    def array_with_doubles(size):
        test_object = [sys.maxsize * rng.random() for _ in range(size)]
        return test_object

    @_register_data("Array with UTF-8 strings")
    def array_with_utf8_strings(size):
        utf8_string = (
            "نظام الحكم سلطاني وراثي "
            "في الذكور من ذرية السيد تركي بن سعيد بن سلطان ويشترط فيمن يختار لولاية"
            " الحكم من بينهم ان يكون مسلما رشيدا عاقلا ًوابنا شرعيا لابوين عمانيين "
        )
        test_object = [utf8_string for _ in range(size)]
        return test_object

    @_register_data("Medium complex object")
    def medium_complex_object(size):
        user = {
            "userId": 3381293,
            "age": 213,
            "username": "johndoe",
            "fullname": "John Doe the Second",
            "isAuthorized": True,
            "liked": 31231.31231202,
            "approval": 31.1471,
            "jobs": [1, 2],
            "currJob": None,
        }
        friends = [user, user, user, user, user, user, user, user]
        test_object = [[user, friends] for _ in range(size)]
        return test_object

    @_register_data("Array with True values")
    def true_values(size):
        test_object = [True for _ in range(size)]
        return test_object

    @_register_data("Array of Dict[str, int]")
    def array_of_dict_string_int(size):
        test_object = [
            {str(rng.random() * 20): int(rng.random() * 1000000)} for _ in range(size)
        ]
        return test_object

    @_register_data("Dict of List[Dict[str, int]]")
    def dict_of_list_dict_str_int(size):
        keys = set()
        while len(keys) < size:
            key = str(rng.random() * 20)
            keys.add(key)
        test_object = {
            key: [
                {str(rng.random() * 20): int(rng.random() * 1000000)}
                for _ in range(256)
            ]
            for key in keys
        }
        return test_object

    @_register_data("Complex object")
    def complex_object(size):
        import json

        # TODO: might be better to reigster this file with setup.py or
        # download it via some mechanism
        try:
            dpath = ub.Path(__file__).parent
            fpath = dpath / "sample.json"
            if not fpath.exists():
                raise Exception
        except Exception:
            import ujson

            dpath = ub.Path(ujson.__file__).parent / "tests"
            fpath = dpath / "sample.json"
            if not fpath.exists():
                raise Exception
        with open(fpath) as f:
            test_object = json.load(f)
        if size is not None:
            test_object = [test_object] * size
        return test_object

    return data_lut
