
import unittest
from unittest import TestCase

import ujson
try:
    import json
except ImportError:
    import simplejson as json
import math
import time
import datetime
import calendar
import StringIO
import re

class UltraJSONTests(TestCase):

    def test_encodeDictWithUnicodeKeys(self):
        input = { u"key1": u"value1", u"key1": u"value1", u"key1": u"value1", u"key1": u"value1", u"key1": u"value1", u"key1": u"value1" }
        output = ujson.encode(input)

        input = { u"بن": u"value1", u"بن": u"value1", u"بن": u"value1", u"بن": u"value1", u"بن": u"value1", u"بن": u"value1", u"بن": u"value1" }
        output = ujson.encode(input)

        pass

    def test_encodeDoubleConversion(self):
        input = math.pi
        output = ujson.encode(input)
        self.assertEquals(round(input, 5), round(json.loads(output), 5))
        self.assertEquals(round(input, 5), round(ujson.decode(output), 5))
        pass
        
    def test_encodeWithDecimal(self):
        input = 1.0
        output = ujson.encode(input)
        self.assertEquals(output, "1.0")

    def test_encodeDoubleNegConversion(self):
        input = -math.pi
        output = ujson.encode(input)
        self.assertEquals(round(input, 5), round(json.loads(output), 5))
        self.assertEquals(round(input, 5), round(ujson.decode(output), 5))
        pass

    def test_encodeArrayOfNestedArrays(self):
        input = [[[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]], [[[]]] ]
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        #self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))

    def test_encodeArrayOfDoubles(self):
        input = [ 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337, 31337.31337 ]
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        #self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))

    def test_encodeStringConversion(self):
        input = "A string \\ \/ \b \f \n \r \t"
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_decodeUnicodeConversion(self):
        pass

    def test_encodeUnicodeConversion1(self):
        input = "Räksmörgås اسامة بن محمد بن عوض بن لادن"
        enc = ujson.encode(input)
        dec = ujson.decode(enc)
        self.assertEquals(enc, json.dumps(input, encoding="utf-8"))
        self.assertEquals(dec, json.loads(enc))
        
    def test_encodeControlEscaping(self):
        input = "\x19"
        enc = ujson.encode(input)
        dec = ujson.decode(enc)
        self.assertEquals(input, dec)
        self.assertEquals(enc, json.dumps(input, encoding="utf-8"))
        

    def test_encodeUnicodeConversion2(self):
        input = "\xe6\x97\xa5\xd1\x88"
        enc = ujson.encode(input)
        dec = ujson.decode(enc)
        self.assertEquals(enc, json.dumps(input, encoding="utf-8"))
        self.assertEquals(dec, json.loads(enc))

    def test_encodeUnicodeSurrogatePair(self):
        input = "\xf0\x90\x8d\x86"
        enc = ujson.encode(input)
        dec = ujson.decode(enc)
                
        self.assertEquals(enc, json.dumps(input, encoding="utf-8"))
        self.assertEquals(dec, json.loads(enc))

    def test_encodeUnicode4BytesUTF8(self):
        input = "\xf0\x91\x80\xb0TRAILINGNORMAL"
        enc = ujson.encode(input)
        dec = ujson.decode(enc)

        self.assertEquals(enc, json.dumps(input, encoding="utf-8"))
        self.assertEquals(dec, json.loads(enc))
            
    def test_encodeUnicode4BytesUTF8Highest(self):
        input = "\xf3\xbf\xbf\xbfTRAILINGNORMAL"
        enc = ujson.encode(input)

        dec = ujson.decode(enc)
                
        self.assertEquals(enc, json.dumps(input, encoding="utf-8"))				
        self.assertEquals(dec, json.loads(enc))

        
    def test_encodeArrayInArray(self):
        input = [[[[]]]]
        output = ujson.encode(input)

        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeIntConversion(self):
        input = 31337
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeIntNegConversion(self):
        input = -31337
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass
        

    def test_encodeLongNegConversion(self):
        input = -9223372036854775808
        output = ujson.encode(input)

        outputjson = json.loads(output)
        outputujson = ujson.decode(output)

        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeListConversion(self):
        input = [ 1, 2, 3, 4 ]
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeDictConversion(self):
        input = { "k1": 1, "k2":  2, "k3": 3, "k4": 4 }
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(input, ujson.decode(output))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeNoneConversion(self):
        input = None
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeTrueConversion(self):
        input = True
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeFalseConversion(self):
        input = False
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeDatetimeConversion(self):
        ts = time.time()
        input = datetime.datetime.fromtimestamp(ts)
        output = ujson.encode(input)
        expected = calendar.timegm(input.utctimetuple())
        self.assertEquals(int(expected), json.loads(output))
        self.assertEquals(int(expected), ujson.decode(output))
        pass

    def test_encodeDateConversion(self):
        ts = time.time()
        input = datetime.date.fromtimestamp(ts)

        output = ujson.encode(input)
        tup = ( input.year, input.month, input.day, 0, 0, 0 )

        expected = calendar.timegm(tup)
        self.assertEquals(int(expected), json.loads(output))
        self.assertEquals(int(expected), ujson.decode(output))
        pass

    def test_encodeRecursionMax(self):
        # 8 is the max recursion depth

        class O2:
            member = 0
            pass	

        class O1:
            member = 0
            pass

        input = O1()
        input.member = O2()
        input.member.member = input

        try:
            output = ujson.encode(input)
            assert False, "Expected overflow exception"
        except(OverflowError):
            pass

    def test_encodeDoubleNan(self):
        input = float('nan')
        try:
            ujson.encode(input)
            assert False, "Expected exception!"
        except(OverflowError):
            return
        assert False, "Wrong exception"
        
    def test_encodeDoubleInf(self):
        input = float('inf')
        try:
            ujson.encode(input)
            assert False, "Expected exception!"
        except(OverflowError):
            return
        assert False, "Wrong exception"
            
    def test_encodeDoubleNegInf(self):
        input = -float('inf')
        try:
            ujson.encode(input)
            assert False, "Expected exception!"
        except(OverflowError):
            return
        assert False, "Wrong exception"
            

    def test_decodeJibberish(self):
        input = "fdsa sda v9sa fdsa"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeBrokenArrayStart(self):
        input = "["
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeBrokenObjectStart(self):
        input = "{"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeBrokenArrayEnd(self):
        input = "]"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeBrokenObjectEnd(self):
        input = "}"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeStringUnterminated(self):
        input = "\"TESTING"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeStringUntermEscapeSequence(self):
        input = "\"TESTING\\\""
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeStringBadEscape(self):
        input = "\"TESTING\\\""
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeTrueBroken(self):
        input = "tru"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeFalseBroken(self):
        input = "fa"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"

    def test_decodeNullBroken(self):
        input = "n"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return
        assert False, "Wrong exception"
            

    def test_decodeBrokenDictKeyTypeLeakTest(self):
        input = '{{1337:""}}'
        for x in xrange(1000):
            try:
                ujson.decode(input)
                assert False, "Expected exception!"
            except(ValueError),e:
                continue

            assert False, "Wrong exception"
            
    def test_decodeBrokenDictLeakTest(self):
        input = '{{"key":"}'
        for x in xrange(1000):
            try:
                ujson.decode(input)
                assert False, "Expected exception!"
            except(ValueError):
                continue

            assert False, "Wrong exception"
            
    def test_decodeBrokenListLeakTest(self):
        input = '[[[true'
        for x in xrange(1000):
            try:
                ujson.decode(input)
                assert False, "Expected exception!"
            except(ValueError):
                continue

            assert False, "Wrong exception"

    def test_decodeDictWithNoKey(self):
        input = "{{{{31337}}}}"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return

        assert False, "Wrong exception"

    def test_decodeDictWithNoColonOrValue(self):
        input = "{{{{\"key\"}}}}"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return

        assert False, "Wrong exception"

    def test_decodeDictWithNoValue(self):
        input = "{{{{\"key\":}}}}"
        try:
            ujson.decode(input)
            assert False, "Expected exception!"
        except(ValueError):
            return

        assert False, "Wrong exception"

    def test_decodeNumericIntPos(self):
        input = "31337"
        self.assertEquals (31337, ujson.decode(input))

    def test_decodeNumericIntNeg(self):
        input = "-31337"
        self.assertEquals (-31337, ujson.decode(input))

    def test_encodeUnicode4BytesUTF8Fail(self):
        input = "\xfd\xbf\xbf\xbf\xbf\xbf"
        try:
            enc = ujson.encode(input)
            assert False, "Expected exception"
        except OverflowError:
            pass
            
    def test_encodeListLongConversion(self):
        input = [9223372036854775807, 9223372036854775807, 9223372036854775807, 9223372036854775807, 9223372036854775807, 9223372036854775807 ]
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_encodeLongConversion(self):
        input = 9223372036854775807
        output = ujson.encode(input)
        self.assertEquals(input, json.loads(output))
        self.assertEquals(output, json.dumps(input))
        self.assertEquals(input, ujson.decode(output))
        pass

    def test_numericIntExp(self):
        input = "1337E40"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))
        
    def test_numericIntFrcExp(self):
        input = "1.337E40"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))

    def test_decodeNumericIntExpEPLUS(self):
        input = "1337E+40"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))

    def test_decodeNumericIntExpePLUS(self):
        input = "1.337e+40"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))

    def test_decodeNumericIntExpE(self):
        input = "1337E40"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))

    def test_decodeNumericIntExpe(self):
        input = "1337e40"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))

    def test_decodeNumericIntExpEMinus(self):
        input = "1.337E-4"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))

    def test_decodeNumericIntExpeMinus(self):
        input = "1.337e-4"
        output = ujson.decode(input)
        self.assertEquals(output, json.loads(input))
        
    def test_encodeBigEscape(self):
        for x in xrange(10):
            input = "\xc3\xa5" * 1024 * 1024 * 10
            output = ujson.encode(input)
    
    def test_decodeBigEscape(self):
        for x in xrange(10):
            input = "\"" + ("\xc3\xa5" * 1024 * 1024 * 10) + "\""
            output = ujson.decode(input)

    def test_dumpToFile(self):
        f = StringIO.StringIO()
        ujson.dump([1, 2, 3], f)
        self.assertEquals("[1,2,3]", f.getvalue())

    def test_dumpToFileLikeObject(self):
        class filelike:
            def __init__(self):
                self.bytes = ''
            def write(self, bytes):
                self.bytes += bytes
        f = filelike()
        ujson.dump([1, 2, 3], f)
        self.assertEquals("[1,2,3]", f.bytes)

    def test_dumpFileArgsError(self):
        try:
            ujson.dump([], '')
        except TypeError:
            pass
        else:
            assert False, 'expected TypeError'

    def test_loadFile(self):
        f = StringIO.StringIO("[1,2,3,4]")
        self.assertEquals([1, 2, 3, 4], ujson.load(f))

    def test_loadFileLikeObject(self):
        class filelike:
            def read(self):
                try:
                    self.end
                except AttributeError:
                    self.end = True
                    return "[1,2,3,4]"
        f = filelike()
        self.assertEquals([1, 2, 3, 4], ujson.load(f))

    def test_loadFileArgsError(self):
        try:
            ujson.load("[]")
        except TypeError:
            pass
        else:
            assert False, "expected TypeError"

    def test_version(self):
        assert re.match(r'^\d+\.\d+(\.\d+)?$', ujson.__version__), \
               "ujson.__version__ must be a string like '1.4.0'"
    
"""
def test_decodeNumericIntFrcOverflow(self):
input = "X.Y"
raise NotImplementedError("Implement this test!")

def test_decodeNumericIntPosOverflow(self):
input = "9223372036854775807322"
try:
    ujson.decode(input)
    assert False, "Expected exception!"
except(ValueError):
    return
assert False, "Wrong exception"

def test_decodeNumericIntNegOverflow(self):
input = "-9223372036854775807322"
try:
    ujson.decode(input)
    assert False, "Expected exception!"
except(ValueError):
    return
assert False, "Wrong exception"

def test_decodeStringUnicodeEscape(self):
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUnicodeBrokenEscape(self):
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUnicodeInvalidEscape(self):
input = "\u3131"
raise NotImplementedError("Implement this test!")

def test_decodeStringUTF8(self):
input = "someutfcharacters"
raise NotImplementedError("Implement this test!")


"""


if __name__ == "__main__":
    unittest.main()
