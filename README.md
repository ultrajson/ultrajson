# UltraJSON

UltraJSON is a fast and extendable JSON encoder and decoder written in pure C.
As of this writing it is faster than any other mainstream JSON library available for Python.

It is part of a family of highly optimized C libraries designed to improve performance of low-level tasks. See amysql and ultramemcache.

## Installing

Currently, only bindings for Python are available (aside from the pure C library).
The easiest way to install is to use the [package available from PyPI](http://pypi.python.org/pypi/ujson/)

To install the UltraJSON Python module, run:

    pip install ujson

```pip``` is very much recommended for managing Python packages. But if you don't have pip installed, try ```easy_install ujson```.

### Building

Instead of installing the packages, you may (optionally) build it from source.

Clone the project from GitHub<br>
Build and install the ujson Python extension

    cd <root>/python
    sudo python setup.py build install

Optionally, you can run the tests<br>
    
    python tests.py

Same instructions applies for Windows except that step 1 isn't necessary since
a prebuilt static library is included.

## Bindings for other languages and platforms

While UltraJSON ships with Python bindings only, no assumptions has been made for the Python environment.
Porting it to other languages and platforms should be no problem. It's just that the UltraJSON developers likes and use Python on a daily basis. If you'd like to port it to Ruby, node.js or Erlang, we would be more than happy to accept such a patch.


## Performance

UltraJSON foremost design goal has been performance. By doing simple things like not iterating a single byte in buffer more than once, extreme performance has been achieved. Also, it doesn't depend on any third-party libraries

Below are 64-bit and 32-bit benchmarks, comparing UltraJSON to other popular JSON serialization libraries.
As mentioned earlier, bindings are currently only available for Python therefore these benchmarks have compared only those available in Python.

### Linux 64-bit benchmark

    Python 2.6.6 (r266:84292, Sep 15 2010, 16:22:56)
    OS Version: Ubuntu 10.10
    System Type: x64-based PC
    Processor: Intel(R) Core(TM) i5-2300 CPU @ 2.80GHz
    Total Physical Memory: 4096 MB


| Test                                               | Type   | ujson     |  cjson    | simplejson |
|--------------------------------------------------- | ------ | --------- | --------- | ---------- |
| Medium complex object                              | Encode | 2534.94   | 2047.95   | 10307.63   |
|                                                    | Decode | 3565.51   | 3575.39   | 7274.10    |
| Array with 256 doubles                             | Encode | 2046.29   | 2133.56   | 26975.49   |
|                                                    | Decode | 4419.08   | 4114.36   | 28430.33   |
| Array with 256 strings                             | Encode | 15736.74  | 6371.26   | 21348.25   |
|                                                    | Decode | 21115.75  | 16468.88  | 26050.25   |
| Array with 256 dict{string, int} pairs             | Encode | 3876.86   | 3050.65   | 14776.41   |
|                                                    | Decode | 7152.09   | 7993.04   | 12934.39   |
| Array with 256 utf-8 strings                       | Encode | 1013.98   | 1040.66   | 1191.98    |
|                                                    | Decode | 269.85    | 493.30    | 1215.66    |
| Array with 256 True values                         | Encode | 34288.36  | 47168.35  | 89846.12   |
|                                                    | Decode | 76296.14  | 58795.91  | 99423.47   |


### Windows 32-bit benchmark
    
    Python 2.6.6 (r266:84297, Aug 24 2010, 18:46:32) [MSC v.1500 32 bit (Intel)]
    OS Version: 6.1.7601 Service Pack 1 Build 7601
    System Type: x64-based PC
    Processor: Intel(R) Core(TM)2 Quad CPU Q9550 @ 2.83GHz 2.83 GHz
    Total Physical Memory: 8191 MB


| Test                                               | Type   | ujson     |  cjson    | simplejson |
|--------------------------------------------------- | ------ | --------- | --------- | ---------- |
| Array with 256 utf-8 strings                       | Encode | 1013.98   | 1040.66   | 1191.98   |
|                                                    | Decode | 269.85    | 493.30    | 1215.66   |
| Medium complex object                              | Encode | 2534.94   | 2047.95   | 10307.63  |
|                                                    | Decode | 3565.51   | 3575.39   | 7274.10   |
| Array with 256 doubles                             | Encode | 2046.29   | 2133.56   | 26975.49  |
|                                                    | Decode | 4419.08   | 4114.36   | 28430.33  |
| Array with 256 strings                             | Encode | 15736.74  | 6371.26   | 21348.25  |
|                                                    | Decode | 21115.75  | 16468.88  | 26050.25  |
| Array with 256 dict{string, int} pairs             | Encode | 3876.86   | 3050.65   | 14776.41  |
|                                                    | Decode | 7152.09   | 7993.04   | 12934.39  |
| Array with 256 True values                         | Encode | 34288.36  | 47168.35  | 89846.12  |
|                                                    | Decode | 76296.14  | 58795.91  | 99423.47  |

### Linux 32-bit benchmark

    CentOS 5.6 (Python 2.4)

| Test                                               | Type   | ujson     |  cjson    | simplejson |
|--------------------------------------------------- | ------ | --------- | --------- | ---------- |
| Dict with 256 arrays with 256 dict{string, int} pairs | Encode | 10.41     | 6.93      | 31.08     |
|                                                    | Decode | 15.05     | 20.31     | 19.81     |
| Medium complex object                              | Encode | 1418.77   | 1252.92   | 6010.21   |
|                                                    | Decode | 2166.18   | 3444.13   | 4637.52   |
| Array with 256 doubles                             | Encode | 1613.39   | 2035.58   | 16300.61  |
|                                                    | Decode | 6199.49   | 5785.33   | 17301.00  |
| Array with 256 strings                             | Encode | 9351.67   | 7786.13   | 12252.28  |
|                                                    | Decode | 6796.77   | 15971.02  | 10951.17  |
| Array with 256 dict{string, int} pairs             | Encode | 2756.91   | 1758.26   | 8811.85   |
|                                                    | Decode | 4161.97   | 6330.77   | 6490.36   |
| Array with 256 utf-8 strings                       | Encode | 658.31    | 62.18     | 1453.30   |
|                                                    | Decode | 124.20    | 455.28    | 1016.58   |
| Array with 256 True values                         | Encode | 18707.57  | 24150.26  | 72618.15  |
|                                                    | Decode | 47098.40  | 48069.53  | 53650.94  |

See [python/benchmark.py](https://github.com/esnme/ultrajson/blob/master/python/benchmark.py) for further information.
