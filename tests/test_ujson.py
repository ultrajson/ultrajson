# coding=UTF-8
from __future__ import print_function, unicode_literals

import decimal
import functools
import json
import math
import re
import sys
from collections import OrderedDict

import pytest
import six
import ujson
from six.moves import range, zip

json_unicode = (
    functools.partial(json.dumps, encoding="utf-8") if six.PY2 else json.dumps
)


def assert_almost_equal(a, b):
    assert round(abs(a - b), 7) == 0


def test_encode_decimal():
    sut = decimal.Decimal("1337.1337")
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert decoded == 1337.1337


def test_encode_string_conversion():
    input = "A string \\ / \b \f \n \r \t </script> &"
    not_html_encoded = '"A string \\\\ \\/ \\b \\f \\n \\r \\t <\\/script> &"'
    html_encoded = (
        '"A string \\\\ \\/ \\b \\f \\n \\r \\t \\u003c\\/script\\u003e \\u0026"'
    )
    not_slashes_escaped = '"A string \\\\ / \\b \\f \\n \\r \\t </script> &"'

    def helper(expected_output, **encode_kwargs):
        output = ujson.encode(input, **encode_kwargs)
        assert output == expected_output
        if encode_kwargs.get("escape_forward_slashes", True):
            assert input == json.loads(output)
            assert input == ujson.decode(output)

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
    input = math.pi
    output = ujson.encode(input)
    assert round(input, 5) == round(json.loads(output), 5)
    assert round(input, 5) == round(ujson.decode(output), 5)


def test_encode_double_neg_conversion():
    input = -math.pi
    output = ujson.encode(input)

    assert round(input, 5) == round(json.loads(output), 5)
    assert round(input, 5) == round(ujson.decode(output), 5)


def test_encode_array_of_nested_arrays():
    input = [[[[]]]] * 20
    output = ujson.encode(input)
    assert input == json.loads(output)
    # assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encode_array_of_doubles():
    input = [31337.31337, 31337.31337, 31337.31337, 31337.31337] * 10
    output = ujson.encode(input)
    assert input == json.loads(output)
    # assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encode_string_conversion2():
    input = "A string \\ / \b \f \n \r \t"
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == '"A string \\\\ \\/ \\b \\f \\n \\r \\t"'
    assert input == ujson.decode(output)


def test_decode_unicode_conversion():
    pass


def test_encode_unicode_conversion1():
    input = "Räksmörgås اسامة بن محمد بن عوض بن لادن"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)
    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encode_control_escaping():
    input = "\x19"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)
    assert input == dec
    assert enc == json_unicode(input)


def test_encode_unicode_conversion2():
    input = "\xe6\x97\xa5\xd1\x88"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)
    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encode_unicode_surrogate_pair():
    input = "\xf0\x90\x8d\x86"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)

    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encode_unicode_4_bytes_utf8():
    input = "\xf0\x91\x80\xb0TRAILINGNORMAL"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)

    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encode_unicode_4_bytes_utf8_highest():
    input = "\xf3\xbf\xbf\xbfTRAILINGNORMAL"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)

    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


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

    # ujson outputs an UTF-8 encoded str object
    if six.PY2:
        encoded = ujson.dumps(s, ensure_ascii=False).decode("utf-8")
    else:
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

    # ujson outputs an UTF-8 encoded str object
    if six.PY2:
        encoded = ujson.dumps(s, ensure_ascii=False).decode("utf-8")
    else:
        encoded = ujson.dumps(s, ensure_ascii=False)

    # json outputs an unicode object
    encoded_json = json.dumps(s, ensure_ascii=False)
    assert len(encoded) == len(s) + 2  # original length + quotes
    assert encoded == encoded_json
    decoded = ujson.loads(encoded)
    assert s == decoded


@pytest.mark.parametrize("test_input", [[[[[]]]], 31337, -31337, None, True, False])
def test_encode_decode(test_input):
    output = ujson.encode(test_input)

    assert test_input == json.loads(output)
    assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)


def test_encode_long_neg_conversion():
    input = -9223372036854775808
    output = ujson.encode(input)

    json.loads(output)
    ujson.decode(output)

    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encode_list_conversion():
    input = [1, 2, 3, 4]
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert input == ujson.decode(output)


def test_encode_dict_conversion():
    input = {"k1": 1, "k2": 2, "k3": 3, "k4": 4}
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert input == ujson.decode(output)
    assert input == ujson.decode(output)


def test_encode_dict_values_ref_counting():
    import gc

    gc.collect()
    value = ["abc"]
    data = {"1": value}
    ref_count = sys.getrefcount(value)
    ujson.dumps(data)
    assert ref_count == sys.getrefcount(value)


def test_encode_to_utf8():
    input = b"\xe6\x97\xa5\xd1\x88"
    if not six.PY2:
        input = input.decode("utf-8")
    enc = ujson.encode(input, ensure_ascii=False)
    dec = ujson.decode(enc)
    assert enc == json.dumps(input, ensure_ascii=False)
    assert dec == json.loads(enc)


def test_decode_from_unicode():
    input = '{"obj": 31337}'
    dec1 = ujson.decode(input)
    dec2 = ujson.decode(str(input))
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

    input = O1()
    input.member = O2()
    input.member.member = input
    with pytest.raises(OverflowError):
        ujson.encode(input)


def test_decode_dict():
    input = "{}"
    obj = ujson.decode(input)
    assert {} == obj
    input = '{"one": 1, "two": 2, "three": 3}'
    obj = ujson.decode(input)
    assert {"one": 1, "two": 2, "three": 3} == obj


def test_encode_unicode_4_bytes_utf8_fail():
    input = b"\xfd\xbf\xbf\xbf\xbf\xbf"
    with pytest.raises(OverflowError):
        ujson.encode(input)


def test_encode_null_character():
    input = "31337 \x00 1337"
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)

    input = "\x00"
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)

    assert '"  \\u0000\\r\\n "' == ujson.dumps("  \u0000\r\n ")


def test_decode_null_character():
    input = '"31337 \\u0000 31337"'
    assert ujson.decode(input) == json.loads(input)


def test_dump_to_file():
    f = six.StringIO()
    ujson.dump([1, 2, 3], f)
    assert "[1,2,3]" == f.getvalue()


def test_dump_to_file_like_object():
    class filelike:
        def __init__(self):
            self.bytes = ""

        def write(self, bytes):
            self.bytes += bytes

    f = filelike()
    ujson.dump([1, 2, 3], f)
    assert "[1,2,3]" == f.bytes


def test_dump_file_args_error():
    with pytest.raises(TypeError):
        ujson.dump([], "")


def test_load_file():
    f = six.StringIO("[1,2,3,4]")
    assert [1, 2, 3, 4] == ujson.load(f)


def test_load_file_like_object():
    class filelike:
        def read(self):
            try:
                self.end
            except AttributeError:
                self.end = True
                return "[1,2,3,4]"

    f = filelike()
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
        '{"id": %s}' % 2 ** 31,
        '{"id": %s}' % 2 ** 32,
        '{"id": %s}' % ((2 ** 32) - 1),
    )
    results = (3590016419, 2 ** 31, 2 ** 32, 2 ** 32 - 1)
    for doc, result in zip(docs, results):
        assert ujson.decode(doc)["id"] == result


def test_encode_big_escape():
    for x in range(10):
        if six.PY2:
            base = "\xc3\xa5"
        else:
            base = "\u00e5".encode("utf-8")
        input = base * 1024 * 1024 * 2
        ujson.encode(input)


def test_decode_big_escape():
    for x in range(10):
        if six.PY2:
            base = "\xc3\xa5"
            quote = '"'
        else:
            base = "\u00e5".encode("utf-8")
            quote = b'"'
        input = quote + (base * 1024 * 1024 * 2) + quote
        ujson.decode(input)


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
    input = "[]"
    obj = ujson.decode(input)
    assert [] == obj


@pytest.mark.skipif(six.PY2, reason="Only raises on Python 3")
def test_encoding_invalid_unicode_character():
    s = "\udc7f"
    with pytest.raises(UnicodeEncodeError):
        ujson.dumps(s)


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
    "test_input, expected", [("31337", 31337), ("-31337", -31337)],
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
    "test_input, expected",
    [
        ('{{1337:""}}', ValueError),  # broken dict key type leak test
        ('{{"key":"}', ValueError),  # broken dict leak test
        ('{{"key":"}', ValueError),  # broken dict leak test
        ("[[[true", ValueError),  # broken list leak test
    ],
)
def test_decode_range_raises(test_input, expected):
    for x in range(1000):
        with pytest.raises(ValueError):
            ujson.decode(test_input)


@pytest.mark.parametrize(
    "test_input, expected",
    [
        ("fdsa sda v9sa fdsa", ValueError),  # jibberish
        ("[", ValueError),  # broken array start
        ("{", ValueError),  # broken object start
        ("]", ValueError),  # broken array end
        ("[" * (1024 * 1024), ValueError),  # array depth too big
        ("}", ValueError),  # broken object end
        ('{"one":1,}', ValueError),  # object trailing comma fail
        ("{" * (1024 * 1024), ValueError),  # object depth too big
        ('"TESTING', ValueError),  # string unterminated
        ('"TESTING\\"', ValueError),  # string bad escape
        ("tru", ValueError),  # true broken
        ("fa", ValueError),  # false broken
        ("n", ValueError),  # null broken
        ("{{{{31337}}}}", ValueError),  # dict with no key
        ('{{{{"key"}}}}', ValueError),  # dict with no colon or value
        ('{{{{"key":}}}}', ValueError),  # dict with no value
        ("[31337,]", ValueError),  # array trailing comma fail
        ("[,31337]", ValueError),  # array leading comma fail
        ("[,]", ValueError),  # array only comma fail
        ("[]]", ValueError),  # array unmatched bracket fail
        ("18446744073709551616", ValueError),  # too big value
        ("-90223372036854775809", ValueError),  # too small value
        ("18446744073709551616", ValueError),  # very too big value
        ("-90223372036854775809", ValueError),  # very too small value
        ("{}\n\t a", ValueError),  # with trailing non whitespaces
        ("[18446744073709551616]", ValueError),  # array with big int
        ('{"age", 44}', ValueError),  # read bad object syntax
    ],
)
def test_decode_raises(test_input, expected):
    with pytest.raises(expected):
        ujson.decode(test_input)


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
        [18446744073709551615, 18446744073709551615, 18446744073709551615],
    ],
)
def test_encode_list_long_conversion(test_input):
    output = ujson.encode(test_input)
    assert test_input == json.loads(output)
    assert test_input == ujson.decode(output)


@pytest.mark.parametrize(
    "test_input", [9223372036854775807, 18446744073709551615],
)
def test_encode_long_conversion(test_input):
    output = ujson.encode(test_input)

    assert test_input == json.loads(output)
    assert output == json.dumps(test_input)
    assert test_input == ujson.decode(output)


@pytest.mark.parametrize(
    "test_input, expected",
    [
        (float("nan"), OverflowError),
        (float("inf"), OverflowError),
        (-float("inf"), OverflowError),
        (12839128391289382193812939, OverflowError),
    ],
)
def test_encode_raises(test_input, expected):
    with pytest.raises(expected):
        ujson.encode(test_input)


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
