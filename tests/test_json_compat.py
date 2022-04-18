r"""

Compare cases

python -c "import json  as json; print(repr(json.dumps({None: None})))"
python -c "import ujson as json; print(repr(json.dumps({None: None})))"

python -c "import json         ; print(repr(json.dumps([1], indent='\x000')))"
python -c "import ujson as json; print(repr(json.dumps([1], indent='\x000')))"

python -c "import json         ; print(repr(json.dumps([1, 2], indent='a \x000 b')))"
python -c "import ujson as json; print(repr(json.dumps([1, 2], indent='a \x000 b')))"

python -c "import json         ; print(repr(json.dumps([1], indent='\udfff')))"
python -c "import  json as json; print(repr(json.dumps([1, 2, 3], indent='\udfff')))"
python -c "import ujson as json; print(repr(json.dumps([1, 2, 3], indent='\udfff')))"

"""

import itertools as it
import json as pjson
from collections import defaultdict

import ujson

JSON_IMPLS = {
    "ujson": ujson,
    "pjson": pjson,
}


def group_items(items, key):
    """
    Groups a list of items by group id. (from ubelt)
    """
    pair_list = ((key(item), item) for item in items)
    # Initialize a dict of lists
    id_to_items = defaultdict(list)
    # Insert each item into the correct group
    for groupid, item in pair_list:
        id_to_items[groupid].append(item)
    return id_to_items


def named_product(basis):
    # Implementation from ubelt
    keys = list(basis.keys())
    for vals in it.product(*basis.values()):
        kw = dict(zip(keys, vals))
        yield kw


def test_dumps_compatability():
    """
    Test the difference between Python's json module (pjson) and ultrajson
    (ujson) under a grid of different parameters.
    """

    # Define the data we will test
    # data = {'a': [1, 2, 3, named_product]}
    data = {"a": [1, 2, 3]}

    # Define the parameters we will test
    NULL_CHAR = "\x00"
    UTF_SURROGATE0000 = "\udc80"
    UTF_SURROGATE1024 = "\udfff"
    param_basis = {
        "indent": [
            -1,
            -2,
            "    ",
            " ab ",
            4,
            0,
            None,
            "\t",
            NULL_CHAR,
            UTF_SURROGATE0000,
            UTF_SURROGATE1024,
        ],
        # 'ensure_ascii': [False],
        "ensure_ascii": [True, False, None],
        "sort_keys": [True, False, None],
        "default": [None, str],
        "module": list(JSON_IMPLS.keys()),
    }
    kwargs_keys = ["indent", "default", "ensure_ascii", "sort_keys"]
    kwargs_keys = [k for k in kwargs_keys if k in param_basis]
    param_grid = named_product(param_basis)
    results = []
    for params in param_grid:
        params_key = pjson.dumps(params, default=str)
        module = JSON_IMPLS[params["module"]]
        kwargs = {k: params[k] for k in kwargs_keys if k in params}
        try:
            result = module.dumps(data, **kwargs)
        except Exception as ex:
            error = ex
            result = None
        else:
            error = 0
        row = {
            "params_key": params_key,
            **params,
            "data": data,
            "result": result,
            "error": error,
        }
        results.append(row)

    print(pjson.dumps(results, indent="    ", default=repr))

    def grouper(row):
        return tuple((k, row[k]) for k in kwargs_keys)

    grouped_results = group_items(results, key=grouper)

    agree_keys = []
    diagree_keys = []

    for group_key, group in grouped_results.items():
        assert len(group) == 2
        module_to_row = {r["module"]: r for r in group}
        assert len(module_to_row) == 2

        ujson_row = module_to_row["ujson"]
        pjson_row = module_to_row["pjson"]

        if ujson_row["error"] and pjson_row["error"]:
            # Both implementations errored
            agree_keys.append(group_key)
        else:
            # Check if the results from all implementations are the same
            agree_keys.append(group_key)
            u_result = ujson_row["result"]
            p_result = pjson_row["result"]

            try:
                p_val = pjson.loads(p_result)
            except Exception as ex:
                p_val = repr(ex)

            try:
                u_val = pjson.loads(u_result)
            except Exception as ex:
                u_val = repr(ex)

            if p_val != u_val:
                import difflib

                print(f"Disagree on {group_key}")
                print(f" * p_result = {p_result!r}")
                print(f" * u_result = {u_result!r}")
                print("".join(list(difflib.ndiff([str(p_val)], [str(u_val)]))))
                diagree_keys.append(group_key)
            else:
                agree_keys.append(group_key)

    print(f"Num Agree: {len(agree_keys)}")
    print(f"Num Disagree: {len(diagree_keys)}")


if __name__ == "__main__":
    """
    CommandLine:
        python ~/code/ultrajson/tests/test_json_compat.py
    """
    test_dumps_compatability()
