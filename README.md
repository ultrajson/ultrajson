# UltraJSON

[![PyPI version](https://img.shields.io/pypi/v/ujson.svg?logo=pypi&logoColor=FFE873)](https://pypi.org/project/ujson)
[![Supported Python versions](https://img.shields.io/pypi/pyversions/ujson.svg?logo=python&logoColor=FFE873)](https://pypi.org/project/ujson)
[![PyPI downloads](https://img.shields.io/pypi/dm/ujson.svg)](https://pypistats.org/packages/ujson)
[![GitHub Actions status](https://github.com/ultrajson/ultrajson/workflows/Test/badge.svg)](https://github.com/ultrajson/ultrajson/actions)
[![codecov](https://codecov.io/gh/ultrajson/ultrajson/branch/main/graph/badge.svg)](https://codecov.io/gh/ultrajson/ultrajson)
[![DOI](https://zenodo.org/badge/1418941.svg)](https://zenodo.org/badge/latestdoi/1418941)
[![Code style: Black](https://img.shields.io/badge/code%20style-Black-000000.svg)](https://github.com/psf/black)

UltraJSON is an ultra fast JSON encoder and decoder written in pure C with bindings for
Python 3.7+.

Install with pip:

```sh
python -m pip install ujson
```

## Usage

May be used as a drop in replacement for most other JSON parsers for Python:

```pycon
>>> import ujson
>>> ujson.dumps([{"key": "value"}, 81, True])
'[{"key":"value"},81,true]'
>>> ujson.loads("""[{"key": "value"}, 81, true]""")
[{'key': 'value'}, 81, True]
```

### Encoder options

#### encode_html_chars

Used to enable special encoding of "unsafe" HTML characters into safer Unicode
sequences. Default is `False`:

```pycon
>>> ujson.dumps("<script>John&Doe", encode_html_chars=True)
'"\\u003cscript\\u003eJohn\\u0026Doe"'
```

#### ensure_ascii

Limits output to ASCII and escapes all extended characters above 127. Default is `True`.
If your end format supports UTF-8, setting this option to false is highly recommended to
save space:

```pycon
>>> ujson.dumps("åäö")
'"\\u00e5\\u00e4\\u00f6"'
>>> ujson.dumps("åäö", ensure_ascii=False)
'"åäö"'
```

#### escape_forward_slashes

Controls whether forward slashes (`/`) are escaped. Default is `True`:

```pycon
>>> ujson.dumps("http://esn.me")
'"http:\\/\\/esn.me"'
>>> ujson.dumps("http://esn.me", escape_forward_slashes=False)
'"http://esn.me"'
```

#### indent

Controls whether indentation ("pretty output") is enabled. Default is `0` (disabled):

```pycon
>>> ujson.dumps({"foo": "bar"})
'{"foo":"bar"}'
>>> print(ujson.dumps({"foo": "bar"}, indent=4))
{
    "foo":"bar"
}
```

## Benchmarks

*UltraJSON* calls/sec compared to other popular JSON parsers with performance gain
specified below each.

### Test machine

Linux 5.15.0-1038-azure x86_64 #45-Ubuntu SMP Mon Apr 24 15:40:42 UTC 2023

### Versions

- CPython 3.11.3 (main, Apr  6 2023, 07:55:46) [GCC 11.3.0]
- ujson        : 0.1.dev1
- orjson       : 3.8.14
- simplejson   : 3.19.1
- json         : 2.0.9

|                                                                               | ujson      | orjson     | simplejson | json       |
|-------------------------------------------------------------------------------|-----------:|-----------:|-----------:|-----------:|
| Array with 256 doubles                                                        |            |            |            |            |
| encode                                                                        |     12,600 |     66,051 |      4,126 |      4,264 |
| decode                                                                        |     19,852 |     64,581 |     10,330 |     10,378 |
| Array with 256 UTF-8 strings                                                  |            |            |            |            |
| encode                                                                        |      2,355 |     24,212 |      2,086 |      2,134 |
| decode                                                                        |      1,898 |      2,878 |        342 |      1,207 |
| Array with 256 strings                                                        |            |            |            |            |
| encode                                                                        |     35,411 |    108,693 |     12,733 |     17,228 |
| decode                                                                        |     23,124 |     60,845 |     30,166 |     27,527 |
| Medium complex object                                                         |            |            |            |            |
| encode                                                                        |      8,713 |     39,394 |      3,331 |      4,573 |
| decode                                                                        |      9,658 |     17,423 |      6,327 |      7,553 |
| Array with 256 True values                                                    |            |            |            |            |
| encode                                                                        |     81,437 |    344,365 |     63,885 |     59,843 |
| decode                                                                        |    157,638 |    270,217 |    120,104 |    121,518 |
| Array with 256 dict{string, int} pairs                                        |            |            |            |            |
| encode                                                                        |     10,942 |     56,421 |      2,527 |      5,743 |
| decode                                                                        |     13,976 |     20,372 |      7,828 |     10,215 |
| Dict with 256 arrays with 256 dict{string, int} pairs                         |            |            |            |            |
| encode                                                                        |         42 |        221 |          8 |         21 |
| decode                                                                        |         35 |         43 |         22 |         27 |
| Dict with 256 arrays with 256 dict{string, int} pairs, outputting sorted keys |            |            |            |            |
| encode                                                                        |         33 |            |          7 |         22 |
| Complex object                                                                |            |            |            |            |
| encode                                                                        |        395 |            |        341 |        376 |
| decode                                                                        |        401 |        530 |        150 |        249 |

Above metrics are in call/sec, larger is better.

## Build options

For those with particular needs, such as Linux distribution packagers, several
build options are provided in the form of environment variables.

### Debugging symbols

#### UJSON_BUILD_NO_STRIP

By default, debugging symbols are stripped on Linux platforms. Setting this
environment variable with a value of `1` or `True` disables this behavior.

### Using an external or system copy of the double-conversion library

These two environment variables are typically used together, something like:

```sh
export UJSON_BUILD_DC_INCLUDES='/usr/include/double-conversion'
export UJSON_BUILD_DC_LIBS='-ldouble-conversion'
```

Users planning to link against an external shared library should be aware of
the ABI-compatibility requirements this introduces when upgrading system
libraries or copying compiled wheels to other machines.

#### UJSON_BUILD_DC_INCLUDES

One or more directories, delimited by `os.pathsep` (same as the `PATH`
environment variable), in which to look for `double-conversion` header files;
the default is to use the bundled copy.

#### UJSON_BUILD_DC_LIBS

Compiler flags needed to link the `double-conversion` library; the default
is to use the bundled copy.
