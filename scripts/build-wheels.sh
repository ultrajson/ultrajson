#!/bin/bash
set -e -x


# Test then Compile wheels
# Skip cp27mu
mkdir -p /io/pip-cache
mkdir -p /io/temp-wheels
for PYBIN in /opt/python/cp3[5678]*/bin; do
    "${PYBIN}/pip" install -q -U setuptools wheel pytest --cache-dir /io/pip-cache
    (cd /io/ && "${PYBIN}/python" -m pip install .)
    (cd /io/ && "${PYBIN}/python" -m pytest)
    (cd /io/ && "${PYBIN}/python" setup.py -q bdist_wheel -d /io/temp-wheels)
#    break
done

"$PYBIN/pip" install -q auditwheel 

# Wheels aren't considered manylinux unless they have been through 
# auditwheel. (Know idea why.) Auditted wheels go in /io/dist/.
mkdir -p /io/dist/
for whl in /io/dist/*.whl; do
    auditwheel repair "$whl" --plat $PLAT -w /io/dist/
        
done




