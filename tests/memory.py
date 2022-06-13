import gc
import sys
import tracemalloc

# exec the first argument to get func() and n
exec_globals = {}
exec(sys.argv[1], exec_globals)
func = exec_globals["func"]
n = int(sys.argv[2]) if sys.argv[2:] else 1

# Pre-run once
try:
    func()
except Exception:
    pass

# Create filter to only report leaks on the 'tracemalloc: measure' line below
filters = []
with open(__file__) as fp:
    for i, line in enumerate(fp, start=1):
        if "tracemalloc: measure" in line:
            filters.append(tracemalloc.Filter(True, __file__, i))

# Clean up and take a snapshot
tracemalloc.start()
gc.collect()
before = tracemalloc.take_snapshot().filter_traces(filters)

# Run
for i in range(n):
    try:
        func()  # tracemalloc: measure
    except Exception:
        pass

# Clean up and second snapshot
gc.collect()
after = tracemalloc.take_snapshot().filter_traces(filters)

# Check that nothing got leaked
diff = after.compare_to(before, "lineno")
if diff:
    for stat in diff:
        print(stat)
    sys.exit(1)
