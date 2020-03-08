# coding=UTF-8
from __future__ import print_function, unicode_literals

import decimal
import functools
import json
import math
import re
import sys

import pytest
import six
import ujson
from six.moves import range, zip

json_unicode = (
    functools.partial(json.dumps, encoding="utf-8") if six.PY2 else json.dumps
)


def assert_almost_equal(a, b):
    assert round(abs(a - b), 7) == 0


def test_encodeDecimal():
    sut = decimal.Decimal("1337.1337")
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert decoded == 1337.1337


def test_encodeStringConversion():
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


def testWriteEscapedString():
    assert "\"\\u003cimg src='\\u0026amp;'\\/\\u003e\"" == ujson.dumps(
        "<img src='&amp;'/>", encode_html_chars=True
    )


def test_doubleLongIssue():
    sut = {"a": -4342969734183514}
    encoded = json.dumps(sut)
    decoded = json.loads(encoded)
    assert sut == decoded
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert sut == decoded


def test_doubleLongDecimalIssue():
    sut = {"a": -12345678901234.56789012}
    encoded = json.dumps(sut)
    decoded = json.loads(encoded)
    assert sut == decoded
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert sut == decoded


def test_encodeDecodeLongDecimal():
    sut = {"a": -528656961.4399388}
    encoded = ujson.dumps(sut)
    ujson.decode(encoded)


def test_decimalDecodeTest():
    sut = {"a": 4.56}
    encoded = ujson.encode(sut)
    decoded = ujson.decode(encoded)
    assert_almost_equal(sut["a"], decoded["a"])


def test_encodeDictWithUnicodeKeys():
    input = {
        "key1": "value1",
        "key1": "value1",
        "key1": "value1",
        "key1": "value1",
        "key1": "value1",
        "key1": "value1",
    }
    ujson.encode(input)

    input = {
        "بن": "value1",
        "بن": "value1",
        "بن": "value1",
        "بن": "value1",
        "بن": "value1",
        "بن": "value1",
        "بن": "value1",
    }
    ujson.encode(input)


def test_encodeDoubleConversion():
    input = math.pi
    output = ujson.encode(input)
    assert round(input, 5) == round(json.loads(output), 5)
    assert round(input, 5) == round(ujson.decode(output), 5)


def test_encodeWithDecimal():
    input = 1.0
    output = ujson.encode(input)
    assert output == "1.0"


def test_encodeDoubleNegConversion():
    input = -math.pi
    output = ujson.encode(input)

    assert round(input, 5) == round(json.loads(output), 5)
    assert round(input, 5) == round(ujson.decode(output), 5)


def test_encodeArrayOfNestedArrays():
    input = [[[[]]]] * 20
    output = ujson.encode(input)
    assert input == json.loads(output)
    # assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeArrayOfDoubles():
    input = [31337.31337, 31337.31337, 31337.31337, 31337.31337] * 10
    output = ujson.encode(input)
    assert input == json.loads(output)
    # assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeStringConversion2():
    input = "A string \\ / \b \f \n \r \t"
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == '"A string \\\\ \\/ \\b \\f \\n \\r \\t"'
    assert input == ujson.decode(output)


def test_decodeUnicodeConversion():
    pass


def test_encodeUnicodeConversion1():
    input = "Räksmörgås اسامة بن محمد بن عوض بن لادن"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)
    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encodeControlEscaping():
    input = "\x19"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)
    assert input == dec
    assert enc == json_unicode(input)


def test_encodeUnicodeConversion2():
    input = "\xe6\x97\xa5\xd1\x88"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)
    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encodeUnicodeSurrogatePair():
    input = "\xf0\x90\x8d\x86"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)

    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encodeUnicode4BytesUTF8():
    input = "\xf0\x91\x80\xb0TRAILINGNORMAL"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)

    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


def test_encodeUnicode4BytesUTF8Highest():
    input = "\xf3\xbf\xbf\xbfTRAILINGNORMAL"
    enc = ujson.encode(input)
    dec = ujson.decode(enc)

    assert enc == json_unicode(input)
    assert dec == json.loads(enc)


# Characters outside of Basic Multilingual Plane(larger than
# 16 bits) are represented as \UXXXXXXXX in python but should be encoded
# as \uXXXX\uXXXX in json.
def testEncodeUnicodeBMP():
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


def testEncodeSymbols():
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


def test_encodeArrayInArray():
    input = [[[[]]]]
    output = ujson.encode(input)

    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeIntConversion():
    input = 31337
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeIntNegConversion():
    input = -31337
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeLongNegConversion():
    input = -9223372036854775808
    output = ujson.encode(input)

    json.loads(output)
    ujson.decode(output)

    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeListConversion():
    input = [1, 2, 3, 4]
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert input == ujson.decode(output)


def test_encodeDictConversion():
    input = {"k1": 1, "k2": 2, "k3": 3, "k4": 4}
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert input == ujson.decode(output)
    assert input == ujson.decode(output)


def test_encodeDictValuesRefCounting():
    import gc

    gc.collect()
    value = ["abc"]
    data = {"1": value}
    ref_count = sys.getrefcount(value)
    ujson.dumps(data)
    assert ref_count == sys.getrefcount(value)


def test_encodeNoneConversion():
    input = None
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeTrueConversion():
    input = True
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeFalseConversion():
    input = False
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeToUTF8():
    input = b"\xe6\x97\xa5\xd1\x88"
    if not six.PY2:
        input = input.decode("utf-8")
    enc = ujson.encode(input, ensure_ascii=False)
    dec = ujson.decode(enc)
    assert enc == json.dumps(input, ensure_ascii=False)
    assert dec == json.loads(enc)


def test_decodeFromUnicode():
    input = '{"obj": 31337}'
    dec1 = ujson.decode(input)
    dec2 = ujson.decode(str(input))
    assert dec1 == dec2


def test_encodeRecursionMax():
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


def test_encodeDoubleNan():
    input = float("nan")
    with pytest.raises(OverflowError):
        ujson.encode(input)


def test_encodeDoubleInf():
    input = float("inf")
    with pytest.raises(OverflowError):
        ujson.encode(input)


def test_encodeDoubleNegInf():
    input = -float("inf")
    with pytest.raises(OverflowError):
        ujson.encode(input)


def test_encodeOrderedDict():
    from collections import OrderedDict

    input = OrderedDict([(1, 1), (0, 0), (8, 8), (2, 2)])
    assert '{"1":1,"0":0,"8":8,"2":2}' == ujson.encode(input)


def test_decodeJibberish():
    input = "fdsa sda v9sa fdsa"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeBrokenArrayStart():
    input = "["
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeBrokenObjectStart():
    input = "{"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeBrokenArrayEnd():
    input = "]"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeArrayDepthTooBig():
    input = "[" * (1024 * 1024)
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeBrokenObjectEnd():
    input = "}"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeObjectTrailingCommaFail():
    input = '{"one":1,}'
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeObjectDepthTooBig():
    input = "{" * (1024 * 1024)
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeStringUnterminated():
    input = '"TESTING'
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeStringUntermEscapeSequence():
    input = '"TESTING\\"'
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeStringBadEscape():
    input = '"TESTING\\"'
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeTrueBroken():
    input = "tru"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeFalseBroken():
    input = "fa"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeNullBroken():
    input = "n"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeBrokenDictKeyTypeLeakTest():
    input = '{{1337:""}}'
    for x in range(1000):
        with pytest.raises(ValueError):
            ujson.decode(input)


def test_decodeBrokenDictLeakTest():
    input = '{{"key":"}'
    for x in range(1000):
        with pytest.raises(ValueError):
            ujson.decode(input)


def test_decodeBrokenListLeakTest():
    input = "[[[true"
    for x in range(1000):
        with pytest.raises(ValueError):
            ujson.decode(input)


def test_decodeDict():
    input = "{}"
    obj = ujson.decode(input)
    assert {} == obj
    input = '{"one": 1, "two": 2, "three": 3}'
    obj = ujson.decode(input)
    assert {"one": 1, "two": 2, "three": 3} == obj


def test_decodeDictWithNoKey():
    input = "{{{{31337}}}}"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeDictWithNoColonOrValue():
    input = '{{{{"key"}}}}'
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeDictWithNoValue():
    input = '{{{{"key":}}}}'
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeNumericIntPos():
    input = "31337"
    assert 31337 == ujson.decode(input)


def test_decodeNumericIntNeg():
    input = "-31337"
    assert -31337 == ujson.decode(input)


def test_encodeUnicode4BytesUTF8Fail():
    input = b"\xfd\xbf\xbf\xbf\xbf\xbf"
    with pytest.raises(OverflowError):
        ujson.encode(input)


def test_encodeNullCharacter():
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


def test_decodeNullCharacter():
    input = '"31337 \\u0000 31337"'
    assert ujson.decode(input) == json.loads(input)


def test_encodeListLongConversion():
    input = [
        9223372036854775807,
        9223372036854775807,
        9223372036854775807,
        9223372036854775807,
        9223372036854775807,
        9223372036854775807,
    ]
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert input == ujson.decode(output)


def test_encodeListLongUnsignedConversion():
    input = [18446744073709551615, 18446744073709551615, 18446744073709551615]
    output = ujson.encode(input)

    assert input == json.loads(output)
    assert input == ujson.decode(output)


def test_encodeLongConversion():
    input = 9223372036854775807
    output = ujson.encode(input)
    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_encodeLongUnsignedConversion():
    input = 18446744073709551615
    output = ujson.encode(input)

    assert input == json.loads(output)
    assert output == json.dumps(input)
    assert input == ujson.decode(output)


def test_numericIntExp():
    input = "1337E40"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_numericIntFrcExp():
    input = "1.337E40"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_decodeNumericIntExpEPLUS():
    input = "1337E+9"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_decodeNumericIntExpePLUS():
    input = "1.337e+40"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_decodeNumericIntExpE():
    input = "1337E40"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_decodeNumericIntExpe():
    input = "1337e40"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_decodeNumericIntExpEMinus():
    input = "1.337E-4"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_decodeNumericIntExpeMinus():
    input = "1.337e-4"
    output = ujson.decode(input)
    assert output == json.loads(input)


def test_dumpToFile():
    f = six.StringIO()
    ujson.dump([1, 2, 3], f)
    assert "[1,2,3]" == f.getvalue()


def test_dumpToFileLikeObject():
    class filelike:
        def __init__(self):
            self.bytes = ""

        def write(self, bytes):
            self.bytes += bytes

    f = filelike()
    ujson.dump([1, 2, 3], f)
    assert "[1,2,3]" == f.bytes


def test_dumpFileArgsError():
    with pytest.raises(TypeError):
        ujson.dump([], "")


def test_loadFile():
    f = six.StringIO("[1,2,3,4]")
    assert [1, 2, 3, 4] == ujson.load(f)


def test_loadFileLikeObject():
    class filelike:
        def read(self):
            try:
                self.end
            except AttributeError:
                self.end = True
                return "[1,2,3,4]"

    f = filelike()
    assert [1, 2, 3, 4] == ujson.load(f)


def test_loadFileArgsError():
    with pytest.raises(TypeError):
        ujson.load("[]")


def test_version():
    assert re.search(
        r"^\d+\.\d+(\.\d+)?", ujson.__version__
    ), "ujson.__version__ must be a string like '1.4.0'"


def test_encodeNumericOverflow():
    with pytest.raises(OverflowError):
        ujson.encode(12839128391289382193812939)


def test_decodeNumberWith32bitSignBit():
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


def test_encodeBigEscape():
    for x in range(10):
        if six.PY2:
            base = "\xc3\xa5"
        else:
            base = "\u00e5".encode("utf-8")
        input = base * 1024 * 1024 * 2
        ujson.encode(input)


def test_decodeBigEscape():
    for x in range(10):
        if six.PY2:
            base = "\xc3\xa5"
            quote = '"'
        else:
            base = "\u00e5".encode("utf-8")
            quote = b'"'
        input = quote + (base * 1024 * 1024 * 2) + quote
        ujson.decode(input)


def test_toDict():
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


def test_object_with_json_unicode():
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


def test_decodeArrayTrailingCommaFail():
    input = "[31337,]"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeArrayLeadingCommaFail():
    input = "[,31337]"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeArrayOnlyCommaFail():
    input = "[,]"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeArrayUnmatchedBracketFail():
    input = "[]]"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeArrayEmpty():
    input = "[]"
    obj = ujson.decode(input)
    assert [] == obj


def test_decodeArrayOneItem():
    input = "[31337]"
    ujson.decode(input)


def test_decodeLongUnsignedValue():
    input = "18446744073709551615"
    ujson.decode(input)


def test_decodeBigValue():
    input = "9223372036854775807"
    ujson.decode(input)


def test_decodeSmallValue():
    input = "-9223372036854775808"
    ujson.decode(input)


def test_decodeTooBigValue():
    input = "18446744073709551616"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeTooSmallValue():
    input = "-90223372036854775809"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeVeryTooBigValue():
    input = "18446744073709551616"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeVeryTooSmallValue():
    input = "-90223372036854775809"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeWithTrailingWhitespaces():
    input = "{}\n\t "
    ujson.decode(input)


def test_decodeWithTrailingNonWhitespaces():
    input = "{}\n\t a"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeArrayWithBigInt():
    input = "[18446744073709551616]"
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_decodeFloatingPointAdditionalTests():
    assert -1.1234567893 == ujson.loads("-1.1234567893")
    assert -1.234567893 == ujson.loads("-1.234567893")
    assert -1.34567893 == ujson.loads("-1.34567893")
    assert -1.4567893 == ujson.loads("-1.4567893")
    assert -1.567893 == ujson.loads("-1.567893")
    assert -1.67893 == ujson.loads("-1.67893")
    assert -1.7893 == ujson.loads("-1.7893")
    assert -1.893 == ujson.loads("-1.893")
    assert -1.3 == ujson.loads("-1.3")

    assert 1.1234567893 == ujson.loads("1.1234567893")
    assert 1.234567893 == ujson.loads("1.234567893")
    assert 1.34567893 == ujson.loads("1.34567893")
    assert 1.4567893 == ujson.loads("1.4567893")
    assert 1.567893 == ujson.loads("1.567893")
    assert 1.67893 == ujson.loads("1.67893")
    assert 1.7893 == ujson.loads("1.7893")
    assert 1.893 == ujson.loads("1.893")
    assert 1.3 == ujson.loads("1.3")


def test_ReadBadObjectSyntax():
    input = '{"age", 44}'
    with pytest.raises(ValueError):
        ujson.decode(input)


def test_ReadTrue():
    assert ujson.loads("true") is True


def test_ReadFalse():
    assert ujson.loads("false") is False


def test_ReadNull():
    assert ujson.loads("null") is None


def test_WriteTrue():
    assert "true" == ujson.dumps(True)


def test_WriteFalse():
    assert "false" == ujson.dumps(False)


def test_WriteNull():
    assert "null" == ujson.dumps(None)


def test_ReadArrayOfSymbols():
    assert [True, False, None] == ujson.loads(" [ true, false,null] ")


def test_WriteArrayOfSymbolsFromList():
    assert "[true,false,null]" == ujson.dumps([True, False, None])


def test_WriteArrayOfSymbolsFromTuple():
    assert "[true,false,null]" == ujson.dumps((True, False, None))


@pytest.mark.skipif(six.PY2, reason="Only raises on Python 3")
def test_encodingInvalidUnicodeCharacter():
    s = "\udc7f"
    with pytest.raises(UnicodeEncodeError):
        ujson.dumps(s)


def test_sortKeys():
    data = {"a": 1, "c": 1, "b": 1, "e": 1, "f": 1, "d": 1}
    sortedKeys = ujson.dumps(data, sort_keys=True)
    assert sortedKeys == '{"a":1,"b":1,"c":1,"d":1,"e":1,"f":1}'


"""
def test_decodeNumericIntFrcOverflow():
input = "X.Y"
raise NotImplementedError("Implement this test!")


def test_decodeStringUnicodeEscape():
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUnicodeBrokenEscape():
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUnicodeInvalidEscape():
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUTF8():
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
