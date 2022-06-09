import sys

import psutil


def run(func, n):
    # Run func once so any static variables etc. are allocated
    try:
        func()
    except Exception:
        pass

    # Take a measurement
    before = psutil.Process().memory_full_info().uss

    # Run n times
    for i in range(n):
        try:
            func()
        except Exception:
            pass

    # Measure again and return success (less than 1% increase)
    after = psutil.Process().memory_full_info().uss

    print(f"USS before: {before}, after: {after}", file=sys.stderr)
    return (after - before) / before < 0.01


if __name__ == "__main__":
    # Usage: python3 memory.py CODE [N]
    #   CODE must define a function 'func'; it will be exec()'d!
    #   N, if present, overrides the number of runs (default: 1000)
    exec_globals = {}
    exec(sys.argv[1], exec_globals)
    n = int(sys.argv[2]) if sys.argv[2:] else 1000
    if not run(exec_globals["func"], n):
        sys.exit(1)
