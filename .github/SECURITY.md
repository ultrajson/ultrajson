# Security Policy

To report a security issue, either report it privately
[on GitHub](https://github.com/ultrajson/ultrajson/security) (preferred) or
through the [Tidelift security contact](https://tidelift.com/security). Do not
report them on the public issue tracker.

## Report guidelines

Due to the insane numbers of (usually long and unintelligible AI generated) reports we're getting, we ask that reporters follow this guide to minimise time wasted for all involved.

### Focus only on the bug

All we really want is the failure type and a minimal example that causes it to happen. Preferably it would be no more than:

    `ujson.dumps()` performs an out of bounds write if its `default` function returns a large negative integer.

    ```python
    import ujson
    ujson.dumps(object(), default=lambda obj: -(10 ** 2000))
    ```
**This should be short enough to remove any temptation to use an AI to write a report**. We will remove reporters as contributors from CVEs if we feel that they haven't contributed more than an LLM prompt and an unfiltered copy+paste.

The rest of the CVE will be written later. Summaries, conclusions, a dive into the git log, long tables of tested versions, who's affected, impact assessments, severity scores, hypothetical exploit scenarios, verification plans, ASAN traces, anything with the words "baseline", "load bearing" or "contract" in it – they're all a waste of time at the triage stage. Please do not include any of this in a report.

Root causes and suggested fixes are welcome but *only* if they have been checked and written (in a human-readable form) by a human – otherwise please leave them out. AIs are (annoyingly) much worse at root-causing an issue than finding them and will therefore *fix* the minimal reproducible example but rarely the real issue.

### Assume we have read CVEs before

We know what an out of bounds write leads to. We're well aware that a segfault can't be caught via `try/except`. Please don't tell us again.

### If reporting an out-of-memory handling issue

Please fake failed memory allocations by replacing the `PyObject_Malloc()` (or equivalent) call with `PyErr_NoMemory(); return NULL;` in the source code then send us the patch (i.e. copy/paste the output of `git diff main`). Examples that call `resource.setrlimit()` and feature a finicky loop are rarely reproducible across machines and impractical to debug.

Note that we are not interested in segfaults from null dereferences as a result of failed memory allocations unless they can be hit reliably enough to make a more efficient denial of service attack than just posting long lists of `"9" * sys.get_int_max_str_digits()`.

### If reporting a memory leak

Our test suite has an optional built-in leak detector which we prefer over examples containing their own use of `tracemalloc` or ASAN.

Add a test function to `tests/test_ujson.py` which contains the `ujson` call to trigger the leak and leave the `tracemalloc`-ing out.

```python
def test_decimal_list_leak():
  # Suppose ujson.dumps leaks when serialing lists of decimals
  ujson.dumps([decimal.Decimal("2.718281828459045")])
```
```
python setup.py build_ext --inplace --force
PYTHONPATH=. pytest -sk test_decimal_list_leak --leak-max-loops=1000
```
Note that the leak detection stabilises much quicker if any supporting class or function definitions are kept outside of the body of the test function.

## After submitting a report

We are extremely under-resourced. Where I live, the average 12 year old gets the same in pocket money as we each get to maintain `ujson`. Please do not make this worse by badgering us with requests for fix timelines. The answer will always be "I have no idea but this conversation is making it later".
