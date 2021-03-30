#!/usr/bin/env bash

# Coverage for ultrajson's C code.
# Usage:
#   CFLAGS="--coverage -O0" python setup.py -q build_ext --inplace -f
#   pytest
#   ./scripts/coverage.sh
# Then inspect the files in the `cov` folder.

# The exact arguments depend on whether we're using LLVM's gcov or GNU's.
unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     gcov_options=(--relative-only);;
    Darwin*)    gcov_options=(--color);;
    *)          echo "Unsupported OS ${unameOut}"; exit 1;;
esac

# The actual gcov instructions:
gcov "${gcov_options[@]}" python/**.c -o build/temp.*/python
gcov "${gcov_options[@]}" lib/**.c -o build/temp.*/lib

# gcov dumps everything in the cwd without any option to change this.
# Manually move the .gcov files to a `cov` folder.
mkdir -p cov
rm -rf cov/*
mv ./**.gcov cov || exit 1

echo Written gcov files to ./cov
