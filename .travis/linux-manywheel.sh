#!/bin/bash -eux

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    if [ $(echo "${PYBIN}" | grep -o '[[:digit:]][[:digit:]]' | head -n 1) -ge 27 ]; then
        "${PYBIN}/pip" wheel /io/ -w wheelhouse/
    fi
done

# Bundle external shared libraries into the wheels
for whl in wheelhouse/*.whl; do
    auditwheel repair "$whl" -w /io/wheelhouse/
done

# Install packages and test
for PYBIN in /opt/python/*/bin/; do
    if [ $(echo "${PYBIN}" | grep -o '[[:digit:]][[:digit:]]' | head -n 1) -ge 27 ]; then
        "${PYBIN}/pip" install ujson --no-index -f /io/wheelhouse
    fi
done

# run tests
for PYBIN in /opt/python/*/bin; do
    if [ $(echo "${PYBIN}" | grep -o '[[:digit:]][[:digit:]]' | head -n 1) -ge 27 ]; then
        "${PYBIN}/pip" install six blist unittest2 pytz
        "${PYBIN}/python" /io/tests/tests.py
    fi
done