import itertools as it
import json as pjson
from collections import defaultdict

import ujson

JSON_IMPLS = {
    "ujson": ujson,  # Our json
    "pjson": pjson,  # Python's json
}


def group_items(items, key):
    """
    Groups a list of items by group id. (Implementation from ubelt)
    """
    pair_list = ((key(item), item) for item in items)
    # Initialize a dict of lists
    id_to_items = defaultdict(list)
    # Insert each item into the correct group
    for groupid, item in pair_list:
        id_to_items[groupid].append(item)
    return id_to_items


def named_product(basis):
    """
    Like itertools.product but with names. (Implementation from ubelt)
    """
    keys = list(basis.keys())
    for vals in it.product(*basis.values()):
        kw = dict(zip(keys, vals))
        yield kw


def check_dumps_compatability():
    """
    Test the difference between Python's json module (pjson) and ultrajson
    (ujson) under a grid of different parameters.
    """
    import difflib
    import pprint

    # Define the data we will test
    data_lut = {}
    data_lut["data0"] = {"a": [1, 2, 3]}
    data_lut["data1"] = {"a": [1, 2, 3, named_product]}
    data_lut["data2"] = []
    data_lut["data3"] = {}
    data_lut["data4"] = [0]
    data_lut["data5"] = {"1": []}

    # Special characters
    NULL_CHAR = "\x00"
    UTF_SURROGATE0000 = "\udc80"
    UTF_SURROGATE1024 = "\udfff"

    class _NoParamType:
        def __str__(cls):
            return "NoParam"

        def __repr__(cls):
            return "NoParam"

    # sentinel, remove this from kwargs if we see it
    NoParam = _NoParamType()

    # Define the parameter grid we will test
    param_basis = {
        "data": list(data_lut.keys()),
        "indent": [
            NoParam,
            None,
            0,
            -1,
            -2,
            "    ",
            " ab ",
            4,
            "\t",
            NULL_CHAR,
            UTF_SURROGATE0000,
            UTF_SURROGATE1024,
        ],
        # 'ensure_ascii': [False],
        "ensure_ascii": [
            True,
            False,
            None,
            NoParam,
        ],
        "sort_keys": [
            True,
            False,
            None,
            NoParam,
        ],
        "default": [
            None,
            str,
            NoParam,
        ],
        "module": list(JSON_IMPLS.keys()),
    }

    kwargs_keys = ["indent", "default", "ensure_ascii", "sort_keys"]
    kwargs_keys = [k for k in kwargs_keys if k in param_basis]
    param_grid = named_product(param_basis)

    # Iterate over grid of parameters and store results
    results = []
    for params in param_grid:
        # Useful if we need a unique key per item, but clutter otherwise
        # params_key = pjson.dumps(params, default=repr)
        module = JSON_IMPLS[params["module"]]
        kwargs = {
            k: params[k]
            for k in kwargs_keys
            if k in params and params[k] is not NoParam
        }
        data = data_lut[params["data"]]
        try:
            result = module.dumps(data, **kwargs)
        except Exception as ex:
            error = ex
            result = None
        else:
            error = 0
        row = {
            # "params_key": params_key,
            **params,
            "result": result,
            "error": error,
        }
        results.append(row)

    pprint.pprint(results, compact=True, width=128, sort_dicts=0)

    # print(pjson.dumps(results, indent=4, default=str))

    # Compare results

    def grouper(row):
        return tuple((k, row[k]) for k in kwargs_keys + ["data"])

    grouped_results = group_items(results, key=grouper)

    agree_keys = []
    diagree_keys = []

    # If the encoding differs, but only via whitespace
    superficial_disagree_keys = []

    for group_key, group in grouped_results.items():
        assert len(group) == 2, f"{len(group)} - {group}"
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

            linediff = list(difflib.ndiff([p_result], [u_result]))
            hasdiff = any(line[0] in "+-?" for line in linediff)
            textdiff = "".join(linediff)

            if p_val != u_val:
                print(f"Disagree on {group_key}")
                print(f" * u_val = {u_val!r}")
                print(f" * p_val = {p_val!r}")
                print(f" * p_result = {p_result!r}")
                print(f" * u_result = {u_result!r}")
                print("--")
                try:
                    print(textdiff)
                except UnicodeEncodeError:
                    print(textdiff.encode(errors="surrogatepass"))
                print("--")
                pprint.pprint(group, compact=True, width=128, sort_dicts=0)
                diagree_keys.append(group_key)
            else:
                if hasdiff:
                    print(f"Superficial disagree on {group_key}")
                    print(f" * u_val = {u_val!r}")
                    print(f" * p_val = {p_val!r}")
                    print(f" * p_result = {p_result!r}")
                    print(f" * u_result = {u_result!r}")
                    print("--")
                    try:
                        print(textdiff)
                    except UnicodeEncodeError:
                        print(textdiff.encode(errors="surrogatepass"))
                    print("--")
                    pprint.pprint(group, compact=True, width=128, sort_dicts=0)
                    superficial_disagree_keys.append(group_key)
                agree_keys.append(group_key)

    print(f"Num Agree: {len(agree_keys)}")
    print(f"Num Disagree: {len(diagree_keys)}")
    print(f"Num Superficial Disagree: {len(superficial_disagree_keys)}")


if __name__ == "__main__":
    """
    CommandLine:
        python ~/code/ultrajson/tests/test_json_compat.py
    """
    check_dumps_compatability()
