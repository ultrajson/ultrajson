#!/usr/bin/env python3
import argparse
import os.path
import re
import subprocess

VERSION = re.compile(r":: Python :: (\d\.\d)$")

EXES = []
with open("setup.py") as f:
    for line in f:
        match = VERSION.search(line)
        if match:
            major_s, minor_s = match[1].split(".")
            major, minor = int(major_s), int(minor_s)
            if (major, minor) < (3, 8):
                EXES.append(f"cp{major}{minor}-cp{major}{minor}m")
            else:
                EXES.append(f"cp{major}{minor}-cp{major}{minor}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("version")
    args = parser.parse_args()

    pkg = f"ujson=={args.version}"

    os.makedirs("dist", exist_ok=True)
    for exe in EXES:
        pip = f"/opt/python/{exe}/bin/pip"

        if subprocess.call(
            (
                # fmt: off
                "docker", "run", "--rm",
                # so files are not root-owned
                "--user", f"{os.getuid()}:{os.getgid()}",
                "--volume", f'{os.path.abspath("dist")}:/dist:rw',
                "quay.io/pypa/manylinux1_x86_64:latest",
                "bash", "-euxc",
                f"{pip} wheel -w /tmp/wheels --no-deps {pkg} && "
                f"auditwheel repair -w /dist /tmp/wheels/*.whl",
                # fmt: on
            )
        ):
            return 1
    return 0


if __name__ == "__main__":
    exit(main())
