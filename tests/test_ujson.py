import datetime as dt
import decimal
import io
import json
import math
import os.path
import re
import subprocess
import sys
import uuid
from collections import OrderedDict
from pathlib import Path

import pytest
import ujson


def assert_almost_equal(a, b):
    assert round(abs(a - b), 7) == 0


def test_encode_decimal():
    sut = decimal.Decimal("1337.1337")
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert decoded == 1337.1337


def test_encode_string_conversion():
    test_input = "A string \\ / \b \f \n \r \t </script> &"
    not_html_encoded = '"A string \\\\ \\/ \\b \\f \\n \\r \\t <\\/script> &"'
    html_encoded = (
        '"A string \\\\ \\/ \\b \\f \\n \\r \\t \\u003c\\/script\\u003e \\u0026"'
    )
    not_slashes_escaped = '"A string \\\\ / \\b \\f \\n \\r \\t </script> &"'

    def helper(expected_output, **encode_kwargs):
        output = ujson.encode(test_input, **encode_kwargs)
        assert output == expected_output
        if encode_kwargs.get("escape_forward_slashes", True):
            assert test_input == json.loads(output)
            assert test_input == ujson.decode(output)

    # Default behavior assumes encode_html_chars=False.
    helper(not_html_encoded, ensure_ascii=True)
    helper(not_html_encoded, ensure_ascii=False)

    # Make sure explicit encode_html_chars=False works.
    helper(not_html_encoded, ensure_ascii=True, encode_html_chars=False)
    helper(not_html_encoded, ensure_ascii=False, encode_html_chars=False)

    # Make sure explicit encode_html_chars=True does the encoding.
    helper(html_encoded, ensure_ascii=True, encode_html_chars=True)
    helper(html_encoded, ensure_ascii=False, encode_html_chars=True)

    # Do escape forward slashes if disabled.
    helper(not_slashes_escaped, escape_forward_slashes=False)


def test_write_escaped_string():
    assert "\"\\u003cimg src='\\u0026amp;'\\/\\u003e\"" == ujson.dumps(
        "<img src='&amp;'/>", encode_html_chars=True
    )


def test_double_long_issue():
    sut = {"a": -4342969734183514}
    encoded = json.dumps(sut)
    decoded = json.loads(encoded)
    assert sut == decoded
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert sut == decoded


def test_double_long_decimal_issue():
    sut = {"a": -12345678901234.56789012}
    encoded = json.dumps(sut)
    decoded = json.loads(encoded)
    assert sut == decoded
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert sut == decoded


# NOTE: can't match exponents -9 to -5; Python 0-pads
@pytest.mark.parametrize("val", [1e-10, 1e-4, 1e10, 1e15, 1e16, 1e30])
def test_encode_float_string_rep(val):
    assert ujson.dumps(val) == json.dumps(val)


def test_encode_decode_long_decimal():
    sut = {"a": -528656961.4399388}
    encoded = ujson.dumps(sut)
    ujson.decode(encoded)


def test_decimal_decode_test():
    sut = {"a": 4.56}
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert_almost_equal(sut["a"], decoded["a"])


def test_encode_double_conversion():
    test_input = math.pi
    output = ujson.encode(test_input)
    assert round(test_input, 5) == round(json.loads(output), 5)
    assert round(test_input, 5) == round(ujson.decode(output), 5)


def test_encode_double_neg_conversion():
    test_input = -math.pi
    output = ujson.encode(test_input)

    assert round(test_input, 5) == round(json.loads(output), 5)
    assert round(test_input, 5) == round(ujson.decode(output), 5)


def test_encode_array_of_nested_arrays():
    test_input = [[[[]]]] * 20
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    # assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)


def test_encode_array_of_doubles():
    test_input = [31337.31337, 31337.31337, 31337.31337, 31337.31337] * 10
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    # assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)


def test_encode_string_conversion2():
    test_input = "A string \\ / \b \f \n \r \t"
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    assert output == '"A string \\\\ \\/ \\b \\f \\n \\r \\t"'
    assert test_input == ujson.decode(output)


def test_encode_control_escaping():
    test_input = "\x19"
    enc = ujson.encode(test_input)
    dec = ujson.decode(enc)
    assert test_input == dec
    assert enc == json.dumps(test_input)


# Characters outside of Basic Multilingual Plane(larger than
# 16 bits) are represented as \UXXXXXXXX in python but should be encoded
# as \uXXXX\uXXXX in json.
def test_encode_unicode_bmp():
    s = "\U0001f42e\U0001f42e\U0001F42D\U0001F42D"  # 🐮🐮🐭🐭
    encoded = ujson.dumps(s)
    encoded_json = json.dumps(s)

    if len(s) == 4:
        assert len(encoded) == len(s) * 12 + 2
    else:
        assert len(encoded) == len(s) * 6 + 2

    assert encoded == encoded_json
    decoded = ujson.loads(encoded)
    assert s == decoded

    # ujson outputs a UTF-8 encoded str object
    encoded = ujson.dumps(s, ensure_ascii=False)

    # json outputs an unicode object
    encoded_json = json.dumps(s, ensure_ascii=False)
    assert len(encoded) == len(s) + 2  # original length + quotes
    assert encoded == encoded_json
    decoded = ujson.loads(encoded)
    assert s == decoded


def test_encode_symbols():
    s = "\u273f\u2661\u273f"  # ✿♡✿
    encoded = ujson.dumps(s)
    encoded_json = json.dumps(s)
    assert len(encoded) == len(s) * 6 + 2  # 6 characters + quotes
    assert encoded == encoded_json
    decoded = ujson.loads(encoded)
    assert s == decoded

    # ujson outputs a UTF-8 encoded str object
    encoded = ujson.dumps(s, ensure_ascii=False)

    # json outputs an unicode object
    encoded_json = json.dumps(s, ensure_ascii=False)
    assert len(encoded) == len(s) + 2  # original length + quotes
    assert encoded == encoded_json
    decoded = ujson.loads(encoded)
    assert s == decoded


def test_encode_long_neg_conversion():
    test_input = -9223372036854775808
    output = ujson.encode(test_input)

    json.loads(output)
    ujson.decode(output)

    assert test_input == json.loads(output)
    assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)


def test_encode_list_conversion():
    test_input = [1, 2, 3, 4]
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    assert test_input == ujson.decode(output)


def test_encode_dict_conversion():
    test_input = {"k1": 1, "k2": 2, "k3": 3, "k4": 4}
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    assert test_input == ujson.decode(output)
    assert test_input == ujson.decode(output)


@pytest.mark.skipif(
    hasattr(sys, "pypy_version_info"), reason="PyPy uses incompatible GC"
)
def test_encode_dict_values_ref_counting():
    import gc

    gc.collect()
    value = ["abc"]
    data = {"1": value}
    ref_count = sys.getrefcount(value)
    ujson.dumps(data)
    assert ref_count == sys.getrefcount(value)


@pytest.mark.skipif(
    hasattr(sys, "pypy_version_info"), reason="PyPy uses incompatible GC"
)
@pytest.mark.parametrize("key", ["key", b"key", 1, True, None])
@pytest.mark.parametrize("sort_keys", [False, True])
def test_encode_dict_key_ref_counting(key, sort_keys):
    import gc

    gc.collect()
    data = {key: "abc"}
    ref_count = sys.getrefcount(key)
    ujson.dumps(data, sort_keys=sort_keys)
    assert ref_count == sys.getrefcount(key)


def test_encode_to_utf8():
    test_input = b"\xe6\x97\xa5\xd1\x88".decode("utf-8")
    enc = ujson.encode(test_input, ensure_ascii=False)
    dec = ujson.decode(enc)
    assert enc == json.dumps(test_input, ensure_ascii=False)
    assert dec == json.loads(enc)


@pytest.mark.parametrize(
    "test_input",
    [
        '{\n    "obj": 31337\n}',
        "{}",
        "[]",
        '{\n    "a": {}\n}',
        "[\n    []\n]",
    ],
)
def test_encode_indent(test_input):
    obj = ujson.decode(test_input)
    output = ujson.encode(obj, indent=4)
    assert test_input == output
    assert output == json.dumps(obj, indent=4)


def test_decode_from_unicode():
    test_input = '{"obj": 31337}'
    dec1 = ujson.decode(test_input)
    dec2 = ujson.decode(str(test_input))
    assert dec1 == dec2


def test_encode_recursion_max():
    # 8 is the max recursion depth
    class O2:
        member = 0

        def toDict(self):
            return {"member": self.member}

    class O1:
        member = 0

        def toDict(self):
            return {"member": self.member}

    test_input = O1()
    test_input.member = O2()
    test_input.member.member = test_input
    with pytest.raises(OverflowError):
        ujson.encode(test_input)


def test_decode_dict():
    test_input = "{}"
    obj = ujson.decode(test_input)
    assert {} == obj
    test_input = '{"one": 1, "two": 2, "three": 3}'
    obj = ujson.decode(test_input)
    assert {"one": 1, "two": 2, "three": 3} == obj


def test_encode_unicode_4_bytes_utf8_fail():
    test_input = b"\xfd\xbf\xbf\xbf\xbf\xbf"
    with pytest.raises(OverflowError):
        ujson.encode(test_input, reject_bytes=False)


def test_encode_null_character():
    test_input = "31337 \x00 1337"
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)

    test_input = "\x00"
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)

    assert '"  \\u0000\\r\\n "' == ujson.dumps("  \u0000\r\n ")


def test_decode_null_character():
    test_input = '"31337 \\u0000 31337"'
    assert ujson.decode(test_input) == json.loads(test_input)


def test_dump_to_file():
    f = io.StringIO()
    ujson.dump([1, 2, 3], f)
    assert "[1,2,3]" == f.getvalue()


def test_dump_to_file_like_object():
    class FileLike:
        def __init__(self):
            self.bytes = ""

        def write(self, bytes):
            self.bytes += bytes

    f = FileLike()
    ujson.dump([1, 2, 3], f)
    assert "[1,2,3]" == f.bytes


def test_dump_file_args_error():
    with pytest.raises(TypeError):
        ujson.dump([], "")


def test_load_file():
    f = io.StringIO("[1,2,3,4]")
    assert [1, 2, 3, 4] == ujson.load(f)


def test_load_file_like_object():
    class FileLike:
        def read(self):
            try:
                self.end
            except AttributeError:
                self.end = True
                return "[1,2,3,4]"

    f = FileLike()
    assert [1, 2, 3, 4] == ujson.load(f)


def test_load_file_args_error():
    with pytest.raises(TypeError):
        ujson.load("[]")


def test_version():
    assert re.search(
        r"^\d+\.\d+(\.\d+)?", ujson.__version__
    ), "ujson.__version__ must be a string like '1.4.0'"


def test_decode_number_with32bit_sign_bit():
    # Test that numbers that fit within 32 bits but would have the
    # sign bit set (2**31 <= x < 2**32) are decoded properly.
    docs = (
        '{"id": 3590016419}',
        '{"id": %s}' % 2**31,
        '{"id": %s}' % 2**32,
        '{"id": %s}' % ((2**32) - 1),
    )
    results = (3590016419, 2**31, 2**32, 2**32 - 1)
    for doc, result in zip(docs, results):
        assert ujson.decode(doc)["id"] == result


def test_encode_big_escape():
    for x in range(10):
        base = "\u00e5".encode()
        test_input = base * 1024 * 1024 * 2
        ujson.encode(test_input, reject_bytes=False)


def test_decode_big_escape():
    for x in range(10):
        base = "\u00e5".encode()
        quote = b'"'
        test_input = quote + (base * 1024 * 1024 * 2) + quote
        ujson.decode(test_input)


def test_to_dict():
    d = {"key": 31337}

    class DictTest:
        def toDict(self):
            return d

        def __json__(self):
            return '"json defined"'  # Fallback and shouldn't be called.

    o = DictTest()
    output = ujson.encode(o)
    dec = ujson.decode(output)
    assert dec == d


def test_object_with_json():
    # If __json__ returns a string, then that string
    # will be used as a raw JSON snippet in the object.
    output_text = "this is the correct output"

    class JSONTest:
        def __json__(self):
            return '"' + output_text + '"'

    d = {"key": JSONTest()}
    output = ujson.encode(d)
    dec = ujson.decode(output)
    assert dec == {"key": output_text}


def test_object_with_complex_json():
    # If __json__ returns a string, then that string
    # will be used as a raw JSON snippet in the object.
    obj = {"foo": ["bar", "baz"]}

    class JSONTest:
        def __json__(self):
            return ujson.encode(obj)

    d = {"key": JSONTest()}
    output = ujson.encode(d)
    dec = ujson.decode(output)
    assert dec == {"key": obj}


def test_object_with_json_type_error():
    # __json__ must return a string, otherwise it should raise an error.
    for return_value in (None, 1234, 12.34, True, {}):

        class JSONTest:
            def __json__(self):
                return return_value

        d = {"key": JSONTest()}
        with pytest.raises(TypeError):
            ujson.encode(d)


def test_object_with_json_attribute_error():
    # If __json__ raises an error, make sure python actually raises it.
    class JSONTest:
        def __json__(self):
            raise AttributeError

    d = {"key": JSONTest()}
    with pytest.raises(AttributeError):
        ujson.encode(d)


def test_decode_array_empty():
    test_input = "[]"
    obj = ujson.decode(test_input)
    assert [] == obj


def test_encode_surrogate_characters():
    assert ujson.dumps("\udc7f") == r'"\udc7f"'
    out = r'{"\ud800":"\udfff"}'
    assert ujson.dumps({"\ud800": "\udfff"}) == out
    assert ujson.dumps({"\ud800": "\udfff"}, sort_keys=True) == out
    o = {b"\xed\xa0\x80": b"\xed\xbf\xbf"}
    assert ujson.dumps(o, reject_bytes=False) == out
    assert ujson.dumps(o, reject_bytes=False, sort_keys=True) == out

    out2 = '{"\ud800":"\udfff"}'
    assert ujson.dumps({"\ud800": "\udfff"}, ensure_ascii=False) == out2
    assert ujson.dumps({"\ud800": "\udfff"}, ensure_ascii=False, sort_keys=True) == out2


@pytest.mark.parametrize(
    "test_input, expected",
    [
        # Normal cases
        (r'"\uD83D\uDCA9"', "\U0001F4A9"),
        (r'"a\uD83D\uDCA9b"', "a\U0001F4A9b"),
        # Unpaired surrogates
        (r'"\uD800"', "\uD800"),
        (r'"a\uD800b"', "a\uD800b"),
        (r'"\uDEAD"', "\uDEAD"),
        (r'"a\uDEADb"', "a\uDEADb"),
        (r'"\uD83D\uD83D\uDCA9"', "\uD83D\U0001F4A9"),
        (r'"\uDCA9\uD83D\uDCA9"', "\uDCA9\U0001F4A9"),
        (r'"\uD83D\uDCA9\uD83D"', "\U0001F4A9\uD83D"),
        (r'"\uD83D\uDCA9\uDCA9"', "\U0001F4A9\uDCA9"),
        (r'"\uD83D \uDCA9"', "\uD83D \uDCA9"),
        # No decoding of actual surrogate characters (rather than escaped ones)
        ('"\uD800"', "\uD800"),
        ('"\uDEAD"', "\uDEAD"),
        ('"\uD800a\uDEAD"', "\uD800a\uDEAD"),
        ('"\uD83D\uDCA9"', "\uD83D\uDCA9"),
    ],
)
def test_decode_surrogate_characters(test_input, expected):
    assert ujson.loads(test_input) == expected
    assert ujson.loads(test_input.encode("utf-8", "surrogatepass")) == expected

    # Ensure that this matches stdlib's behaviour
    assert json.loads(test_input) == expected


def test_sort_keys():
    data = {"a": 1, "c": 1, "b": 1, "e": 1, "f": 1, "d": 1}
    sorted_keys = ujson.dumps(data, sort_keys=True)
    assert sorted_keys == '{"a":1,"b":1,"c":1,"d":1,"e":1,"f":1}'


@pytest.mark.parametrize(
    "test_input",
    [
        "[31337]",  # array one item
        "18446744073709551615",  # long unsigned value
        "9223372036854775807",  # big value
        "-9223372036854775808",  # small value
        "{}\n\t ",  # trailing whitespaces
    ],
)
def test_decode_no_assert(test_input):
    ujson.decode(test_input)


@pytest.mark.parametrize(
    "test_input, expected",
    [
        ("31337", 31337),
        ("-31337", -31337),
        ("100000000000000000000.0", 1e20),
    ],
)
def test_decode(test_input, expected):
    assert ujson.decode(test_input) == expected


@pytest.mark.parametrize(
    "test_input",
    [
        "1337E40",
        "1.337E40",
        "1337E+9",
        "1.337e+40",
        "1337E40",
        "1337e40",
        "1.337E-4",
        "1.337e-4",
    ],
)
def test_decode_numeric_int_exp(test_input):
    output = ujson.decode(test_input)
    assert output == json.loads(test_input)


@pytest.mark.parametrize(
    "i",
    [
        -(10**25),  # very negative
        -(2**64),  # too large in magnitude for a uint64
        -(2**63) - 1,  # too small for a int64
        2**64,  # too large for a uint64
        10**25,  # very positive
    ],
)
@pytest.mark.parametrize("mode", ["encode", "decode"])
def test_encode_decode_big_int(i, mode):
    # Test ints that are too large to be represented by a C integer type
    for python_object in (i, [i], {"i": i}):
        json_string = json.dumps(python_object, separators=(",", ":"))
        if mode == "encode":
            if hasattr(sys, "pypy_version_info"):
                # https://foss.heptapod.net/pypy/pypy/-/issues/3765
                pytest.skip("PyPy can't serialise big ints")
            assert ujson.encode(python_object) == json_string
            if isinstance(python_object, dict):
                assert ujson.encode(python_object, sort_keys=True) == json_string
        else:
            assert ujson.decode(json_string) == python_object


@pytest.mark.parametrize(
    "test_input, expected",
    [
        ('{{1337:""}}', ujson.JSONDecodeError),  # broken dict key type leak test
        ('{{"key":"}', ujson.JSONDecodeError),  # broken dict leak test
        ('{{"key":"}', ujson.JSONDecodeError),  # broken dict leak test
        ("[[[true", ujson.JSONDecodeError),  # broken list leak test
    ],
)
def test_decode_range_raises(test_input, expected):
    for x in range(1000):
        with pytest.raises(expected):
            ujson.decode(test_input)


@pytest.mark.parametrize(
    "test_input, expected",
    [
        ("fdsa sda v9sa fdsa", ujson.JSONDecodeError),  # gibberish
        ("[", ujson.JSONDecodeError),  # broken array start
        ("{", ujson.JSONDecodeError),  # broken object start
        ("]", ujson.JSONDecodeError),  # broken array end
        ("}", ujson.JSONDecodeError),  # broken object end
        ('{"one":1,}', ujson.JSONDecodeError),  # object trailing comma fail
        ('"TESTING', ujson.JSONDecodeError),  # string unterminated
        ('"TESTING\\"', ujson.JSONDecodeError),  # string bad escape
        ("tru", ujson.JSONDecodeError),  # true broken
        ("fa", ujson.JSONDecodeError),  # false broken
        ("n", ujson.JSONDecodeError),  # null broken
        ("{{{{31337}}}}", ujson.JSONDecodeError),  # dict with no key
        ('{{{{"key"}}}}', ujson.JSONDecodeError),  # dict with no colon or value
        ('{{{{"key":}}}}', ujson.JSONDecodeError),  # dict with no value
        ("[31337,]", ujson.JSONDecodeError),  # array trailing comma fail
        ("[,31337]", ujson.JSONDecodeError),  # array leading comma fail
        ("[,]", ujson.JSONDecodeError),  # array only comma fail
        ("[]]", ujson.JSONDecodeError),  # array unmatched bracket fail
        ("{}\n\t a", ujson.JSONDecodeError),  # with trailing non whitespaces
        ('{"age", 44}', ujson.JSONDecodeError),  # read bad object syntax
    ],
)
def test_decode_raises(test_input, expected):
    with pytest.raises(expected):
        ujson.decode(test_input)


@pytest.mark.parametrize(
    "test_input, expected",
    [
        ("[", ujson.JSONDecodeError),  # array depth too big
        ("{", ujson.JSONDecodeError),  # object depth too big
    ],
)
def test_decode_raises_for_long_input(test_input, expected):
    with pytest.raises(expected):
        ujson.decode(test_input * (1024 * 1024))


def test_decode_exception_is_value_error():
    assert issubclass(ujson.JSONDecodeError, ValueError)
    assert ujson.JSONDecodeError is not ValueError


@pytest.mark.parametrize(
    "test_input, expected",
    [
        (True, "true"),
        (False, "false"),
        (None, "null"),
        ([True, False, None], "[true,false,null]"),
        ((True, False, None), "[true,false,null]"),
    ],
)
def test_dumps(test_input, expected):
    assert ujson.dumps(test_input) == expected


class SomeObject:
    def __init__(self, message, exception=None):
        self._message = message
        self._exception = exception

    def __repr__(self):
        if self._exception:
            raise self._exception
        return self._message


@pytest.mark.parametrize(
    "test_input, expected_exception, expected_message",
    [
        (set(), TypeError, "set() is not JSON serializable"),
        ({1, 2, 3}, TypeError, "{1, 2, 3} is not JSON serializable"),
        (SomeObject("Some Object"), TypeError, "Some Object is not JSON serializable"),
        (SomeObject("\ud800"), UnicodeEncodeError, None),
        (SomeObject(None, KeyboardInterrupt), KeyboardInterrupt, None),
    ],
)
def test_dumps_raises(test_input, expected_exception, expected_message):
    with pytest.raises(expected_exception) as e:
        ujson.dumps(test_input)
    if expected_message:
        assert str(e.value) == expected_message


@pytest.mark.parametrize(
    "test_input, expected_exception",
    [
        (float("nan"), OverflowError),
        (float("inf"), OverflowError),
        (-float("inf"), OverflowError),
    ],
)
def test_encode_raises_allow_nan(test_input, expected_exception):
    with pytest.raises(expected_exception):
        ujson.dumps(test_input, allow_nan=False)


def test_nan_inf_support():
    # Test ported from pandas
    text = '["a", NaN, "NaN", Infinity, "Infinity", -Infinity, "-Infinity"]'
    data = ujson.loads(text)
    expected = [
        "a",
        float("nan"),
        "NaN",
        float("inf"),
        "Infinity",
        -float("inf"),
        "-Infinity",
    ]
    for a, b in zip(data, expected):
        assert a == b or math.isnan(a) and math.isnan(b)


def test_special_singletons():
    pos_inf = ujson.loads("Infinity")
    neg_inf = ujson.loads("-Infinity")
    nan = ujson.loads("NaN")
    null = ujson.loads("null")
    assert math.isinf(pos_inf) and pos_inf > 0
    assert math.isinf(neg_inf) and neg_inf < 0
    assert math.isnan(nan)
    assert null is None


@pytest.mark.parametrize(
    "test_input, expected_message",
    [
        ("n", "Unexpected character .* 'null'"),
        ("N", "Unexpected character .*'NaN'"),
        ("NA", "Unexpected character .* 'NaN'"),
        ("Na N", "Unexpected character .* 'NaN'"),
        ("nan", "Unexpected character .* 'null'"),
        ("none", "Unexpected character .* 'null'"),
        ("i", "Expected object or value"),
        ("I", "Unexpected character .* 'Infinity'"),
        ("Inf", "Unexpected character .* 'Infinity'"),
        ("InfinitY", "Unexpected character .* 'Infinity'"),
        ("-i", "Trailing data"),
        ("-I", "Unexpected character .* '-Infinity'"),
        ("-Inf", "Unexpected character .* '-Infinity'"),
        ("-InfinitY", "Unexpected character .* '-Infinity'"),
        ("- i", "Trailing data"),
        ("- I", "Trailing data"),
        ("- Inf", "Trailing data"),
        ("- InfinitY", "Trailing data"),
    ],
)
def test_incomplete_special_inputs(test_input, expected_message):
    with pytest.raises(ujson.JSONDecodeError, match=expected_message):
        ujson.loads(test_input)


@pytest.mark.parametrize(
    "test_input, expected_message",
    [
        ("NaNaNaN", "Trailing data"),
        ("Infinity and Beyond", "Trailing data"),
        ("-Infinity-and-Beyond", "Trailing data"),
        ("NaN!", "Trailing data"),
        ("Infinity!", "Trailing data"),
        ("-Infinity!", "Trailing data"),
    ],
)
def test_overcomplete_special_inputs(test_input, expected_message):
    with pytest.raises(ujson.JSONDecodeError, match=expected_message):
        ujson.loads(test_input)


@pytest.mark.parametrize(
    "test_input",
    [
        {
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
        },
        {
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
        },
    ],
)
def test_encode_no_assert(test_input):
    ujson.encode(test_input)


@pytest.mark.parametrize(
    "test_input, expected",
    [
        (1.0, "1.0"),
        (OrderedDict([(1, 1), (0, 0), (8, 8), (2, 2)]), '{"1":1,"0":0,"8":8,"2":2}'),
        ({"a": float("NaN")}, '{"a":NaN}'),
        ({"a": float("inf")}, '{"a":Infinity}'),
        ({"a": -float("inf")}, '{"a":-Infinity}'),
    ],
)
def test_encode(test_input, expected):
    assert ujson.encode(test_input) == expected


@pytest.mark.parametrize(
    "test_input",
    [
        [
            9223372036854775807,
            9223372036854775807,
            9223372036854775807,
            9223372036854775807,
            9223372036854775807,
            9223372036854775807,
        ],
        [
            18446744073709551615,
            18446744073709551615,
            18446744073709551615,
        ],
    ],
)
def test_encode_list_long_conversion(test_input):
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    assert test_input == ujson.decode(output)


@pytest.mark.parametrize(
    "test_input",
    [
        9223372036854775807,
        18446744073709551615,
    ],
)
def test_encode_long_conversion(test_input):
    output = ujson.encode(test_input)

    assert test_input == json.loads(output)
    assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)


@pytest.mark.parametrize(
    "test_input",
    [
        [[[[]]]],
        31337,
        -31337,
        None,
        True,
        False,
    ],
)
def test_encode_decode(test_input):
    output = ujson.encode(test_input)

    assert test_input == json.loads(output)
    assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)


@pytest.mark.parametrize(
    "test_input",
    [
        "Räksmörgås اسامة بن محمد بن عوض بن لادن",
        "\xe6\x97\xa5\xd1\x88",
        "\xf0\x90\x8d\x86",  # surrogate pair
        "\xf0\x91\x80\xb0TRAILINGNORMAL",  # 4 bytes UTF8
        "\xf3\xbf\xbf\xbfTRAILINGNORMAL",  # 4 bytes UTF8 highest
    ],
)
def test_encode_unicode(test_input):
    enc = ujson.encode(test_input)
    dec = ujson.decode(enc)

    assert enc == json.dumps(test_input)
    assert dec == json.loads(enc)


@pytest.mark.parametrize(
    "test_input, expected",
    [
        ("-1.1234567893", -1.1234567893),
        ("-1.234567893", -1.234567893),
        ("-1.34567893", -1.34567893),
        ("-1.4567893", -1.4567893),
        ("-1.567893", -1.567893),
        ("-1.67893", -1.67893),
        ("-1.7893", -1.7893),
        ("-1.893", -1.893),
        ("-1.3", -1.3),
        ("1.1234567893", 1.1234567893),
        ("1.234567893", 1.234567893),
        ("1.34567893", 1.34567893),
        ("1.4567893", 1.4567893),
        ("1.567893", 1.567893),
        ("1.67893", 1.67893),
        ("1.7893", 1.7893),
        ("1.893", 1.893),
        ("1.3", 1.3),
        ("true", True),
        ("false", False),
        ("null", None),
        (" [ true, false,null] ", [True, False, None]),
    ],
)
def test_loads(test_input, expected):
    assert ujson.loads(test_input) == expected


def test_reject_bytes_default():
    data = {"a": b"b"}
    with pytest.raises(TypeError):
        ujson.dumps(data)


def test_reject_bytes_true():
    data = {"a": b"b"}
    with pytest.raises(TypeError):
        ujson.dumps(data, reject_bytes=True)


def test_reject_bytes_false():
    data = {"a": b"b"}
    assert ujson.dumps(data, reject_bytes=False) == '{"a":"b"}'


def test_encode_none_key():
    data = {None: None}
    assert ujson.dumps(data) == '{"null":null}'


def test_default_function():
    iso8601_time_format = "%Y-%m-%dT%H:%M:%S.%f"

    class CustomObject:
        pass

    class UnjsonableObject:
        pass

    def default(value):
        if isinstance(value, dt.datetime):
            return value.strftime(iso8601_time_format)
        elif isinstance(value, uuid.UUID):
            return value.hex
        elif isinstance(value, CustomObject):
            raise ValueError("invalid value")
        return value

    now = dt.datetime.now()
    expected_output = '"%s"' % now.strftime(iso8601_time_format)
    assert ujson.dumps(now, default=default) == expected_output

    uuid4 = uuid.uuid4()
    expected_output = '"%s"' % uuid4.hex
    assert ujson.dumps(uuid4, default=default) == expected_output

    custom_obj = CustomObject()
    with pytest.raises(ValueError, match="invalid value"):
        ujson.dumps(custom_obj, default=default)

    unjsonable_obj = UnjsonableObject()
    with pytest.raises(TypeError, match="maximum recursion depth exceeded"):
        ujson.dumps(unjsonable_obj, default=default)


@pytest.mark.parametrize("indent", list(range(65537, 65542)))
def test_dump_huge_indent(indent):
    ujson.encode({"a": True}, indent=indent)


@pytest.mark.parametrize("first_length", list(range(2, 7)))
@pytest.mark.parametrize("second_length", list(range(10919, 10924)))
def test_dump_long_string(first_length, second_length):
    ujson.dumps(["a" * first_length, "\x00" * second_length])


def test_dump_indented_nested_list():
    a = _a = []
    for i in range(20):
        _a.append(list(range(i)))
        _a = _a[-1]
        ujson.dumps(a, indent=i)


@pytest.mark.parametrize("indent", [0, 1, 2, 4, 5, 8, 49])
def test_issue_334(indent):
    path = Path(__file__).with_name("334-reproducer.json")
    a = ujson.loads(path.read_bytes())
    ujson.dumps(a, indent=indent)


@pytest.mark.skipif(
    hasattr(sys, "pypy_version_info"), reason="PyPy uses incompatible GC"
)
def test_default_ref_counting():
    class DefaultRefCountingClass:
        def __init__(self, value):
            self._value = value

        def convert(self):
            if self._value > 1:
                return type(self)(self._value - 1)
            return 0

    import gc

    gc.collect()
    ujson.dumps(DefaultRefCountingClass(3), default=lambda x: x.convert())
    assert not any(
        type(o).__name__ == "DefaultRefCountingClass" for o in gc.get_objects()
    )


@pytest.mark.parametrize("sort_keys", [False, True])
def test_obj_str_exception(sort_keys):
    class Obj:
        def __str__(self):
            raise NotImplementedError

    with pytest.raises(NotImplementedError):
        ujson.dumps({Obj(): 1}, sort_keys=sort_keys)


def no_memory_leak(func_code, n=None):
    code = f"import functools, ujson; func = {func_code}"
    path = os.path.join(os.path.dirname(__file__), "memory.py")
    n = [str(n)] if n is not None else []
    p = subprocess.run([sys.executable, path, code] + n)
    assert p.returncode == 0


@pytest.mark.skipif(
    hasattr(sys, "pypy_version_info"), reason="PyPy uses incompatible GC"
)
@pytest.mark.parametrize("input", ['["a" * 11000, b""]'])
def test_no_memory_leak_encoding_errors(input):
    no_memory_leak(f"functools.partial(ujson.dumps, {input})")


@pytest.mark.parametrize(
    "separators, expected",
    [
        (None, '{"a":0,"b":1}'),
        ((",", ":"), '{"a":0,"b":1}'),
        ((", ", ": "), '{"a": 0, "b": 1}'),
        # And some weird values, even though they produce invalid JSON
        (("\u203d", "\u00a1"), '{"a"\u00a10\u203d"b"\u00a11}'),
        (("i\x00", "k\x00"), '{"a"k\x000i\x00"b"k\x001}'),
        (("\udc80", "\udc81"), '{"a"\udc810\udc80"b"\udc811}'),
    ],
)
def test_separators(separators, expected):
    assert ujson.dumps({"a": 0, "b": 1}, separators=separators) == expected


@pytest.mark.parametrize(
    "separators, expected_exception",
    [
        (True, TypeError),
        (0, TypeError),
        (b"", TypeError),
        ((), ValueError),
        ((",",), ValueError),
        ((",", ":", "x"), ValueError),
        ((True, 0), TypeError),
        ((",", True), TypeError),
        ((True, ":"), TypeError),
        ((b",", b":"), TypeError),
    ],
)
def test_separators_errors(separators, expected_exception):
    with pytest.raises(expected_exception):
        ujson.dumps({"a": 0, "b": 1}, separators=separators)


"""
def test_decode_numeric_int_frc_overflow():
input = "X.Y"
raise NotImplementedError("Implement this test!")


def test_decode_string_unicode_escape():
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decode_string_unicode_broken_escape():
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decode_string_unicode_invalid_escape():
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decode_string_utf8():
input = "someutfcharacters"
raise NotImplementedError("Implement this test!")

"""

"""
# Use this to look for memory leaks
if __name__ == '__main__':
    import unittest
    from guppy import hpy
    hp = hpy()
    hp.setrelheap()
    while True:
        try:
            unittest.main()
        except SystemExit:
            pass
        heap = hp.heapu()
        print(heap)
"""
