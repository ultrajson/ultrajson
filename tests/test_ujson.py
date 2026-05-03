import copy
import datetime as dt
import decimal
import enum
import gc
import io
import json
import math
import os.path
import random
import re
import string
import subprocess
import sys
import uuid
from collections import OrderedDict
from pathlib import Path

import pytest

import ujson


class _DefaultLeaker:
    __slots__ = ("payload",)

    def __init__(self):
        self.payload = b"x" * 1024


class _FailingWriteFile:
    def write(self, data):
        raise OSError("fail")


def assert_almost_equal(a, b):
    assert round(abs(a - b), 7) == 0


def test_encode_decimal():
    sut = decimal.Decimal("1337.1337")
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert decoded == 1337.1337


@pytest.mark.skipif(
    sys.implementation.name != "pypy",
    reason="this branch only executes on PyPy (CPython uses module state)",
)
def test_decimal_type_ref_not_leaked_on_pypy():
    # On PyPy, object_is_decimal_type() imports the decimal module on every
    # call. Smoke-test that repeated Decimal encoding doesn't crash or raise,
    # which would indicate that the per-call import is still leaking badly
    # enough to exhaust memory.
    for _ in range(1000):
        ujson.dumps(decimal.Decimal("3.14"))


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


@pytest.mark.parametrize(
    "char, escape",
    [
        ("<", "\\u003c"),
        (">", "\\u003e"),
        ("&", "\\u0026"),
    ],
)
def test_html_chars_encoded_individually(char, escape):
    encoded = ujson.dumps(char, encode_html_chars=True)
    assert escape in encoded.lower()
    assert ujson.loads(encoded) == char


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


@pytest.mark.parametrize(
    "val",
    [
        sys.float_info.max,
        -sys.float_info.max,
        sys.float_info.min,
        sys.float_info.epsilon,
        0.0,
    ],
)
def test_encode_float_boundary_values(val):
    encoded = ujson.dumps(val)
    decoded = ujson.loads(encoded)
    if val == 0.0:
        assert decoded == 0.0
    elif val > 0:
        assert decoded > 0
    else:
        assert decoded < 0


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


@pytest.mark.parametrize("codepoint", range(0x00, 0x20))
def test_encode_control_escaping(codepoint):
    # All 32 control characters must roundtrip and agree with stdlib json.
    ch = chr(codepoint)
    enc = ujson.encode(ch)
    assert ujson.decode(enc) == ch
    assert enc == json.dumps(ch)


# Characters outside of Basic Multilingual Plane(larger than
# 16 bits) are represented as \UXXXXXXXX in python but should be encoded
# as \uXXXX\uXXXX in json.
def test_encode_unicode_bmp():
    s = "\U0001f42e\U0001f42e\U0001f42d\U0001f42d"  # 🐮🐮🐭🐭
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


@pytest.mark.parametrize("test_input", [-1, -9223372036854775808, 18446744073709551615])
def test_encode_special_longs(test_input):
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
    sys.implementation.name in ("pypy", "graalpy"),
    reason="PyPy & GraalPy use incompatible GC",
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
    sys.implementation.name in ("pypy", "graalpy"),
    reason="PyPy & GraalPy use incompatible GC",
)
@pytest.mark.parametrize("key", ["key", b"key", 1, True, False, None])
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


class O1:
    member = 0

    def toDict(self):
        return {"member": self.member}


@pytest.mark.skip_leak_test  # Known memory leak
def test_encode_recursion_max():
    # 8 is the max recursion depth
    test_input = O1()
    test_input.member = O1()
    test_input.member.member = test_input
    with pytest.raises((OverflowError, RecursionError)):
        ujson.encode(test_input)


def test_encoder_deep_nesting_raises():
    # Deeply nested lists trigger the same recursion limit.
    nested = 1
    for _ in range(1025):
        nested = [nested]
    with pytest.raises(OverflowError):
        ujson.dumps(nested)


def test_encoder_nesting_just_under_limit():
    nested = 1
    for _ in range(1023):
        nested = [nested]
    assert ujson.loads(ujson.dumps(nested)) is not None


@pytest.mark.skipif(
    sys.implementation.name in ("pypy", "graalpy"),
    reason="gc.get_objects not reliable on alternative runtimes",
)
def test_no_ref_leak_from_default_recursion_cap():
    # When default= hits DEFAULT_FN_MAX_DEPTH the INVALID label in
    # Object_beginTypeContext must release pc->newObj; without the fix
    # each call leaks one object.
    def chained(obj):
        return _DefaultLeaker()

    for _ in range(10):  # warm-up
        try:
            ujson.dumps(_DefaultLeaker(), default=chained)
        except TypeError:
            pass
    gc.collect()
    before = sum(1 for o in gc.get_objects() if isinstance(o, _DefaultLeaker))

    for _ in range(200):
        try:
            ujson.dumps(_DefaultLeaker(), default=chained)
        except TypeError:
            pass
    gc.collect()
    after = sum(1 for o in gc.get_objects() if isinstance(o, _DefaultLeaker))

    assert after - before < 5, (
        f"Leaked {after - before} _DefaultLeaker instances after 200 iterations"
    )


@pytest.mark.skipif(
    sys.implementation.name in ("pypy", "graalpy"),
    reason="PyPy and GraalPy do not support tracemalloc",
)
def test_no_memory_leak_dump_write_failure():
    # The encoded JSON string must be released on the write-failure path
    # in objToJSONFile; without the fix, each call leaks the encoded string.
    import tracemalloc

    data = {"key": "v" * 2000}
    tracemalloc.start()
    snap1 = tracemalloc.take_snapshot()

    for _ in range(500):
        try:
            ujson.dump(data, _FailingWriteFile())
        except OSError:
            pass

    snap2 = tracemalloc.take_snapshot()
    tracemalloc.stop()

    total_growth = sum(
        s.size_diff for s in snap2.compare_to(snap1, "lineno") if s.size_diff > 0
    )
    assert total_growth < 500 * 1024, (
        f"Leaked {total_growth} bytes across 500 failing writes"
    )


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


def test_dump_load_unicode_roundtrip():
    data = {"emoji": "🐍", "cjk": "日本語", "arabic": "مرحبا"}
    buf = io.StringIO()
    ujson.dump(data, buf)
    buf.seek(0)
    assert ujson.load(buf) == data


class WritableFileLike:
    def __init__(self):
        self.bytes = ""

    def write(self, bytes):
        self.bytes += bytes


def test_dump_to_file_like_object():
    f = WritableFileLike()
    ujson.dump([1, 2, 3], f)
    assert "[1,2,3]" == f.bytes


def test_dump_file_args_error():
    with pytest.raises(TypeError):
        ujson.dump([], "")


def test_dump_write_failure_raises():
    # OSError from file.write() must propagate; the encoded string must be
    # released on this error path (companion to test_no_memory_leak_dump_write_failure).
    with pytest.raises(OSError):
        ujson.dump({"key": "value"}, _FailingWriteFile())


def test_load_file():
    f = io.StringIO("[1,2,3,4]")
    assert [1, 2, 3, 4] == ujson.load(f)


class ReadableFileLike:
    def read(self):
        try:
            self.end
        except AttributeError:
            self.end = True
            return "[1,2,3,4]"


def test_load_file_like_object():
    f = ReadableFileLike()
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


class DictTest:
    def toDict(self):
        return dict(key=31337)

    def __json__(self):
        return '"json defined"'  # Fallback and shouldn't be called.


def test_to_dict():
    o = DictTest()
    output = ujson.encode(o)
    dec = ujson.decode(output)
    assert dec == {"key": 31337}


class _PlainObject:
    pass


def test_default_function_fallthrough():
    # Objects with neither toDict nor __json__ fall through to default=.
    assert (
        ujson.loads(ujson.dumps(_PlainObject(), default=lambda o: "fallback"))
        == "fallback"
    )


class JSONTest:
    def __init__(self, output):
        self.output = output

    def __json__(self):
        return copy.deepcopy(self.output)


def test_object_with_json():
    # If __json__ returns a string, then that string
    # will be used as a raw JSON snippet in the object.
    d = {"key": JSONTest('"this is the correct output"')}
    output = ujson.encode(d)
    dec = ujson.decode(output)
    assert dec == {"key": "this is the correct output"}


def test_object_with_complex_json():
    # If __json__ returns a string, then that string
    # will be used as a raw JSON snippet in the object.
    obj = {"foo": ["bar", "baz"]}
    d = {"key": JSONTest(ujson.dumps(obj))}
    output = ujson.encode(d)
    dec = ujson.decode(output)
    assert dec == {"key": obj}


class _BytesJSON:
    def __json__(self):
        return b'{"source": "bytes"}'


def test_object_with_json_bytes():
    # __json__ may return bytes; they are treated as raw JSON just like str.
    assert ujson.loads(ujson.dumps(_BytesJSON())) == {"source": "bytes"}


def test_object_with_json_type_error():
    # __json__ must return a string, otherwise it should raise an error.
    for return_value in (None, 1234, 12.34, True, {}):
        d = {"key": JSONTest(return_value)}
        with pytest.raises(TypeError):
            ujson.encode(d)


class JSONTestAttributeError:
    def __json__(self):
        raise AttributeError


def test_object_with_json_attribute_error():
    # If __json__ raises an error, make sure python actually raises it.
    d = {"key": JSONTestAttributeError()}
    with pytest.raises(AttributeError):
        ujson.encode(d)


class JSONtoDict:
    def __init__(self, value):
        self.value = value

    def toDict(self):
        return copy.deepcopy(self.value)


def test_object_with_to_dict_type_error():
    # toDict must return a dict, otherwise it should raise an error.
    for return_value in (None, 1234, 12.34, True, "json"):
        d = {"key": JSONtoDict(return_value)}
        with pytest.raises(TypeError):
            ujson.encode(d)


class JSONtoDictAttributeError:
    def toDict(self):
        raise AttributeError


def test_object_with_to_dict_attribute_error():
    # If toDict raises an error, make sure python actually raises it.
    d = {"key": JSONTestAttributeError()}
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


@pytest.mark.xfail(
    sys.implementation.name == "graalpy",
    reason="GraalPy bug, it fails the last example.",
)
@pytest.mark.parametrize(
    "test_input, expected",
    [
        # Normal cases
        (r'"\uD83D\uDCA9"', "\U0001f4a9"),
        (r'"a\uD83D\uDCA9b"', "a\U0001f4a9b"),
        # Unpaired surrogates
        (r'"\uD800"', "\ud800"),
        (r'"a\uD800b"', "a\ud800b"),
        (r'"\uDEAD"', "\udead"),
        (r'"a\uDEADb"', "a\udeadb"),
        (r'"\uD83D\uD83D\uDCA9"', "\ud83d\U0001f4a9"),
        (r'"\uDCA9\uD83D\uDCA9"', "\udca9\U0001f4a9"),
        (r'"\uD83D\uDCA9\uD83D"', "\U0001f4a9\ud83d"),
        (r'"\uD83D\uDCA9\uDCA9"', "\U0001f4a9\udca9"),
        (r'"\uD83D \uDCA9"', "\ud83d \udca9"),
        # No decoding of actual surrogate characters (rather than escaped ones)
        ('"\ud800"', "\ud800"),
        ('"\udead"', "\udead"),
        ('"\ud800a\udead"', "\ud800a\udead"),
        ('"\ud83d\udca9"', "\ud83d\udca9"),
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


def test_sort_keys_unordered():
    data = {"a": 1, 1: 2, None: 3}
    assert ujson.dumps(data) == '{"a":1,"1":2,"null":3}'
    with pytest.raises(TypeError):
        ujson.dumps(data, sort_keys=True)


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
        ("-0", 0),  # negative zero integer → plain 0
        ("-0.0", -0.0),  # negative zero float preserves sign bit
    ],
)
def test_decode(test_input, expected):
    result = ujson.decode(test_input)
    assert result == expected
    if test_input == "-0.0":
        assert math.copysign(1.0, result) < 0


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
        2**100,  # well beyond 64-bit range
        -(2**100),
        2**200,
    ],
)
@pytest.mark.parametrize("mode", ["encode", "decode"])
@pytest.mark.skip_leak_test  # To be fixed separately
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


@pytest.mark.skipif(
    not hasattr(sys, "set_int_max_str_digits"),
    reason="sys.set_int_max_str_digits backported to 3.11, 3.10.7+, 3.9.14+",
)
@pytest.mark.xfail(
    sys.implementation.name == "pypy",
    reason="PyPy's PyNumber_ToBase ignores sys.get_int_max_str_digits()",
)
def test_encode_too_big_int_error():
    with pytest.raises(ValueError, match="integer string conversion"):
        ujson.dumps(pow(10, 10_000))


@pytest.mark.skipif(
    not hasattr(sys, "set_int_max_str_digits"),
    reason="sys.set_int_max_str_digits backported to 3.11, 3.10.7+, 3.9.14+",
)
def test_decode_too_big_int_error():
    with pytest.raises(ValueError, match="integer string conversion"):
        ujson.loads("9" * 10_000)


def test_decode_integer_near_str_digit_limit():
    """
    sys.set_int_max_str_digits (default 4300) was introduced in Python 3.11
    and backported as a security fix to 3.10.7, 3.9.14, and 3.8.14
    (CVE-2020-10735).  ujson's PyLong_FromString path inherits this limit.

    When the limit is present: exactly 4300 digits succeeds; 4301 raises.
    When absent (e.g. 3.10.0–3.10.6): both digit counts decode successfully.
    PyPy does not enforce the limit even when the attribute exists.
    """
    at_limit = "9" * 4300
    over_limit = "9" * 4301

    assert ujson.loads(at_limit) == int(at_limit)

    if hasattr(sys, "set_int_max_str_digits"):
        with pytest.raises(ValueError, match="[Ll]imit|digits"):
            ujson.loads(over_limit)
    else:
        assert ujson.loads(over_limit) == int(over_limit)


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
        ('"\\q"', ujson.JSONDecodeError),  # unrecognised escape letter
        ('"\\x41"', ujson.JSONDecodeError),  # hex escape (Python syntax, not JSON)
        ('"\\0"', ujson.JSONDecodeError),  # octal-style escape (not JSON)
        ('"\\a"', ujson.JSONDecodeError),  # Python bell escape (not JSON)
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


def test_decode_depth_limit_arrays():
    # 1025 levels of nesting exceeds JSON_MAX_OBJECT_DEPTH.
    limit = 1025
    deep = "[" * limit + "1" + "]" * limit
    with pytest.raises(ujson.JSONDecodeError, match="[Dd]epth|[Ll]imit"):
        ujson.loads(deep)


def test_decode_just_under_depth_limit():
    depth = 1024
    deep = "[" * depth + "1" + "]" * depth
    result = ujson.loads(deep)
    for _ in range(depth):
        result = result[0]
    assert result == 1


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
    "test_input",
    [
        float("nan"),
        float("inf"),
        -float("inf"),
        [float("nan")],  # nested in list
        {"k": float("inf")},  # nested in dict value
        [[float("nan")]],  # doubly nested
        {"a": {"b": float("inf")}},
    ],
)
def test_encode_raises_allow_nan(test_input):
    with pytest.raises(OverflowError):
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
    "codepoint, expected_ord",
    [
        ("\\uFFFF", 0xFFFF),  # highest BMP non-surrogate
        ("\\u0080", 0x0080),  # first codepoint needing a 2-byte UTF-8 sequence
        ("\\u0800", 0x0800),  # first codepoint needing a 3-byte UTF-8 sequence
    ],
)
def test_unicode_boundary_codepoints(codepoint, expected_ord):
    decoded = ujson.loads(f'"{codepoint}"')
    assert ord(decoded) == expected_ord


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


@pytest.mark.parametrize(
    "value",
    [
        [b"hello"],  # bytes inside a list
        {"a": {"b": [b"deep"]}},  # bytes deeply nested
    ],
)
def test_reject_bytes_nested(value):
    # reject_bytes=True (default) applies at any nesting depth.
    with pytest.raises(TypeError, match="reject_bytes is on and"):
        ujson.dumps(value)


def test_encode_special_keys():
    data = {None: 0, True: 1, False: 2}
    assert ujson.dumps(data) == '{"null":0,"true":1,"false":2}'
    data = {None: 0}
    assert ujson.dumps(data, sort_keys=True) == '{"null":0}'
    data = {True: 1, False: 2}
    assert ujson.dumps(data, sort_keys=True) == '{"false":2,"true":1}'


class TestDefaultFunction:
    iso8601_time_format = "%Y-%m-%dT%H:%M:%S.%f"

    class CustomObject:
        pass

    class UnjsonableObject:
        pass

    @classmethod
    def default(cls, value):
        if isinstance(value, dt.datetime):
            return value.strftime(cls.iso8601_time_format)
        elif isinstance(value, uuid.UUID):
            return value.hex
        elif isinstance(value, cls.CustomObject):
            raise ValueError("invalid value")
        return value

    def test_datetime(self):
        now = dt.datetime.now()
        expected_output = '"%s"' % now.strftime(self.iso8601_time_format)
        assert ujson.dumps(now, default=self.default) == expected_output

    def test_uuid(self):
        uuid4 = uuid.uuid4()
        expected_output = '"%s"' % uuid4.hex
        assert ujson.dumps(uuid4, default=self.default) == expected_output

    def test_exception_in_default(self):
        custom_obj = self.CustomObject()
        with pytest.raises(ValueError, match="invalid value"):
            ujson.dumps(custom_obj, default=self.default)

    def test_exception_type_preserved(self):
        # Any exception type raised by default= must propagate unchanged.
        class Boom(Exception):
            pass

        with pytest.raises(Boom):
            ujson.dumps(object(), default=lambda o: (_ for _ in ()).throw(Boom()))

    @pytest.mark.skip_leak_test  # Known memory leak
    def test_recursive_default(self):
        unjsonable_obj = self.UnjsonableObject()
        with pytest.raises(TypeError, match="maximum recursion depth exceeded"):
            ujson.dumps(unjsonable_obj, default=self.default)


@pytest.mark.parametrize("indent", [999, 1000, 1001, 1 << 30, 1 << 63, 1 << 128])
def test_dump_huge_indent(indent):
    obj = {"list": [1, [2, 3], 4], "nested": {"key": "value", "a": True}}
    if indent <= 1000:
        assert ujson.loads(ujson.encode(obj, indent=indent)) == obj
    else:
        with pytest.raises((ValueError, OverflowError)):
            ujson.encode(obj, indent=indent)


def test_negative_indent():
    obj = {"a": [1, 2], "b": "c"}
    assert ujson.dumps(obj) == '{"a":[1,2],"b":"c"}'
    assert ujson.dumps(obj, 0) == '{"a":[1,2],"b":"c"}'
    assert ujson.dumps(obj, indent=-1) == '{"a": [1,2],"b": "c"}'
    assert ujson.dumps(obj, indent=-1000000) == '{"a": [1,2],"b": "c"}'
    assert (
        ujson.dumps(obj, indent=2) == '{\n  "a": [\n    1,\n    2\n  ],\n  "b": "c"\n}'
    )


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
    sys.implementation.name in ("pypy", "graalpy"),
    reason="PyPy & GraalPy use incompatible GC",
)
@pytest.mark.skip_leak_test  # Does its own leak checking
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


@pytest.mark.skipif(
    sys.implementation.name == "graalpy",
    reason="GraalPy has an incompatible stub implementation of getrefcount",
)
@pytest.mark.parametrize("sort_keys", [False, True])
@pytest.mark.skip_leak_test  # Does its own leak detection
def test_obj_str_exception(sort_keys):
    class Obj:
        def __str__(self):
            raise NotImplementedError

    key = Obj()
    getrefcount = getattr(sys, "getrefcount", lambda x: 0)
    old = getrefcount(key)
    with pytest.raises(NotImplementedError):
        ujson.dumps({key: 1}, sort_keys=sort_keys)
    assert getrefcount(key) == old


@pytest.mark.skipif(
    sys.implementation.name == "graalpy", reason="GraalPy does not support tracemalloc"
)
def no_memory_leak(func_code, n=None):
    code = f"import functools, ujson; func = {func_code}"
    path = os.path.join(os.path.dirname(__file__), "memory.py")
    n = [str(n)] if n is not None else []
    p = subprocess.run([sys.executable, path, code] + n)
    assert p.returncode == 0


@pytest.mark.skipif(
    sys.implementation.name == "graalpy", reason="GraalPy does not support tracemalloc"
)
def test_no_memory_leak_default_ascii():
    rng = random.Random(631)
    value = "ascii: " + "".join(rng.choice(string.ascii_letters) for _ in range(32))
    data = {"x": object()}

    ujson.dumps(data, ensure_ascii=True, default=lambda o: value)


@pytest.mark.skipif(
    sys.implementation.name == "graalpy", reason="GraalPy does not support tracemalloc"
)
def test_no_memory_leak_default_non_ascii():
    rng = random.Random(631)
    non_ascii_chars = "生日快乐中文测试汉字"
    non_ascii_value = "non_ascii: " + "".join(
        rng.choice(non_ascii_chars) for _ in range(8)
    )
    data = {"x": object()}

    ujson.dumps(data, ensure_ascii=True, default=lambda o: non_ascii_value)


@pytest.mark.skipif(
    sys.implementation.name in ("pypy", "graalpy"),
    reason="PyPy & GraalPy use incompatible GC",
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


@pytest.mark.parametrize("value", [{}, []])
def test_separators_empty_collection(value):
    # Custom separators do not affect the compact form of empty collections.
    assert ujson.dumps(value, separators=(" , ", " : ")) == ujson.dumps(value)


def test_separators_with_indent_roundtrip():
    data = {"x": [1, 2], "y": 3}
    result = ujson.dumps(data, indent=2, separators=(",", ": "))
    assert ujson.loads(result) == data


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


def test_loads_bytes_like():
    assert ujson.loads(b"123") == 123
    if hasattr(sys, "pypy_version_info"):
        with pytest.raises(TypeError, match="PyPy"):
            ujson.loads(memoryview(b"{}"))
    else:
        assert ujson.loads(memoryview(b'["a", "b", "c"]')) == ["a", "b", "c"]
    assert ujson.loads(bytearray(b"99")) == 99
    assert ujson.loads('"🦄🐳"'.encode()) == "🦄🐳"
    # array.array exports the C-contiguous buffer protocol and must be accepted.
    import array as _array

    if not hasattr(sys, "pypy_version_info"):
        assert ujson.loads(_array.array("B", b'{"a":1}')) == {"a": 1}


@pytest.mark.skipif(
    sys.implementation.name in ("pypy", "graalpy"),
    reason="PyPy & GraalPy use incompatible GC",
)
def test_loads_bytes_like_refcounting():
    import gc

    gc.collect()
    buffer = b'{"a": 99}'
    old = sys.getrefcount(buffer)
    assert ujson.loads(buffer) == {"a": 99}
    assert sys.getrefcount(buffer) == old

    buffer = b'{"a": invalid}'
    old = sys.getrefcount(buffer)
    with pytest.raises(ValueError):
        ujson.loads(buffer)
    assert sys.getrefcount(buffer) == old


@pytest.mark.skipif(
    sys.implementation.name == "graalpy",
    reason="GraalPy does not support buffer protocol for non C-contiguous buffers",
)
def test_loads_non_c_contiguous():
    buffer = memoryview(b"".join(bytes([i]) + b"_" for i in b"[1, 2, 3]"))[::2]
    assert not buffer.c_contiguous
    assert ujson.loads(bytes(buffer)) == [1, 2, 3]
    with pytest.raises(TypeError):
        ujson.loads(buffer)


class ExampleIntEnum(enum.IntEnum):
    FOO = 42


class ExampleFloatEnum(float, enum.Enum):
    FOO = 3.1416


@pytest.mark.parametrize(
    "enum_class, expected",
    [(ExampleIntEnum, "42"), (ExampleFloatEnum, "3.1416")],
)
def test_enum(enum_class, expected):
    assert ujson.dumps(enum_class.FOO) == expected


def test_nested_json_decode_error():
    """Parsing a nested json string raises JSONDecodeError and not SystemError."""

    # Test that nested JSON with invalid format raises JSONDecodeError
    with pytest.raises(ujson.JSONDecodeError):
        ujson.loads(b'{{"a":"b"}:"c"}')

    # Test with another nested JSON case
    with pytest.raises(ujson.JSONDecodeError):
        ujson.loads('{"a":{"b":"c"}:"d"}')

    # Test that JSONDecodeError is a subclass of ValueError
    assert issubclass(ujson.JSONDecodeError, ValueError)


def test_comprehensive_json_fixture():
    """
    Loads comprehensive.json — a fixture that exercises every JSON value type
    at multiple nesting levels

      * ujson agrees with stdlib json on the decoded Python objects.
      * A ujson roundtrip (loads → dumps → loads) is lossless.
      * Each JSON type decodes to the correct Python type.
      * Specific deeply-nested values are reachable and correct.

    The test also exercises ujson's documented tolerances for non-standard
    JSON that stdlib json rejects:
      • NaN / Infinity / -Infinity as bare value tokens
      • Integers with leading zeros  (e.g. "01" → 1)
      • Trailing-decimal floats      (e.g. "1." → 1.0)
      • \\/ as an escape for '/'     (allowed by the spec, used in the fixture)
    """
    fixture = Path(__file__).with_name("comprehensive.json")
    raw = fixture.read_bytes()

    # Agreement with stdlib json
    data = ujson.loads(raw)
    stdlib_data = json.loads(raw)
    assert data == stdlib_data

    # Roundtrip stability
    assert ujson.loads(ujson.dumps(data)) == data

    # Type correctness for every JSON primitive type
    nums = data["numbers"]
    assert isinstance(nums["zero"], int) and nums["zero"] == 0
    assert isinstance(nums["forty_two"], int) and nums["forty_two"] == 42
    assert isinstance(nums["negative_one"], int) and nums["negative_one"] == -1
    assert isinstance(nums["pi"], float) and math.isclose(
        nums["pi"], math.pi, rel_tol=1e-12
    )
    assert isinstance(nums["half"], float) and nums["half"] == 0.5
    assert isinstance(nums["small_float"], float)

    bools = data["booleans"]
    assert bools["yes"] is True
    assert bools["no"] is False

    assert data["null_value"] is None

    assert isinstance(data["strings"]["plain"], str)
    assert data["strings"]["empty"] == ""

    # String escape sequences survived the roundtrip
    escapes = data["strings"]["standard_escapes"]
    assert "\t" in escapes  # \t
    assert "\n" in escapes  # \n
    assert "\r" in escapes  # \r
    assert "\b" in escapes  # \b
    assert "\f" in escapes  # \f
    assert "\\" in escapes  # \\
    assert '"' in escapes  # \"
    assert "/" in escapes  # \/ (solidus — optional escape, still valid)

    # Unicode strings
    assert "é" in data["strings"]["unicode_latin"]
    assert "中" in data["strings"]["unicode_cjk"]
    assert "😀" in data["strings"]["unicode_emoji"]

    # All array variants
    arrs = data["arrays"]
    assert arrs["empty"] == []
    assert arrs["single"] == [42]
    assert arrs["integers"] == list(range(1, 11))
    assert arrs["mixed"][0] == 1
    assert arrs["mixed"][1] == "two"
    assert arrs["mixed"][4] is False
    assert arrs["mixed"][5] is None
    assert arrs["mixed"][6] == []
    assert arrs["mixed"][7] == {}
    assert arrs["nested_4"][0][0][0] == [1, 2]  # 4 levels deep
    assert arrs["of_objects"][1]["val"] == "b"

    # All object variants
    objs = data["objects"]
    assert objs["empty"] == {}
    assert objs["single_key"] == {"only": "value"}
    assert objs["flat"]["d"] is None

    deep = objs["nested_6"]["l1"]["l2"]["l3"]["l4"]["l5"]["l6"]
    assert deep == "six levels deep"
    assert objs["nested_6"]["l1"]["l2"]["l3"]["l4"]["l5"]["l6_int"] == 6
    assert objs["nested_6"]["l1"]["l2"]["l3"]["l4"]["l5"]["l6_arr"] == [6, 6, 6]

    # Heavily-nested records (objects inside arrays inside objects)
    alice = data["records"][0]
    assert alice["name"] == "Alice"
    assert alice["active"] is True
    assert alice["score"] == 98.5
    assert alice["tags"] == ["admin", "superuser", "reviewer"]
    assert alice["address"]["coords"]["lat"] == 39.7817
    assert alice["prefs"]["limits"]["rate"] == 10.5
    assert alice["history"][1]["action"] == "upload"
    assert alice["metadata"] is None

    bob = data["records"][1]
    assert bob["active"] is False
    assert bob["tags"] == []
    assert bob["metadata"]["flags"] == [1, 2, 4]

    # Matrix (array of arrays of ints)
    assert data["matrix"][3][4] == 20  # bottom-right corner
    assert sum(data["matrix"][0]) == 15  # first row

    # Config subtree
    cfg = data["config"]
    assert cfg["features"]["beta"]["suboptions"]["backoff"] == [1.0, 2.0, 4.0, 8.0]
    assert cfg["limits"]["burst"] is None
    assert "example.com" in cfg["allowed_origins"][0]

    # ujson-specific tolerances (non-standard JSON that ujson accepts)
    nan_doc = '{"values": [NaN, Infinity, -Infinity]}'
    tol = ujson.loads(nan_doc)
    assert math.isnan(tol["values"][0])
    assert math.isinf(tol["values"][1]) and tol["values"][1] > 0
    assert math.isinf(tol["values"][2]) and tol["values"][2] < 0
    # Both ujson and stdlib json agree.
    stdlib_tol = json.loads(nan_doc)
    assert math.isnan(stdlib_tol["values"][0])
    assert tol["values"][1] == stdlib_tol["values"][1]
    assert tol["values"][2] == stdlib_tol["values"][2]

    # Leading-zero integer: "01" → 1 (RFC 8259 §6 violation).
    # stdlib json has always rejected this; ujson is permissive.
    with pytest.raises(json.JSONDecodeError):
        json.loads("01")
    assert ujson.loads("01") == 1

    # Trailing-decimal float: "1." → 1.0 (not valid JSON, but ujson tolerates it).
    with pytest.raises(json.JSONDecodeError):
        json.loads("1.")
    assert ujson.loads("1.") == 1.0


"""
The following checks are not part of the standard test suite.
They can be run manually as follows:
python -c 'from tests.test_ujson import check_foo; check_foo()'
"""


def check_decode_decimal_no_int_overflow():
    # Requires enough free RAM to hold a ~4GB string in memory
    decoded = ujson.decode(r'[0.123456789,"{}"]'.format("a" * (2**32 - 5)))
    assert decoded[0] == 0.123456789


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
