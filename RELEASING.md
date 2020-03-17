# Release Checklist

- [ ] Get master to the appropriate code release state.
      [GitHub Actions](https://github.com/ultrajson/ultrajson/actions) and
      [Travis CI](https://travis-ci.com/ultrajson/ultrajson) should be running
      cleanly for all merges to master.
      [![GitHub Actions status](https://github.com/ultrajson/ultrajson/workflows/Test/badge.svg)](https://github.com/ultrajson/ultrajson/actions)
      [![Build Status](https://travis-ci.org/ultrajson/ultrajson.svg?branch=master)](https://travis-ci.org/ultrajson/ultrajson)

- [ ] Edit release draft, adjust text if needed: https://github.com/ultrajson/ultrajson/releases

- [ ] Check next tag is correct, amend if needed

- [ ] Publish release

- [ ] Check the tagged
      [GitHub Actions build](https://github.com/ultrajson/ultrajson/actions?query=workflow%3ADeploy)
      has deployed to [PyPI](https://pypi.org/project/ujson/#history)

* [ ] Create wheels in a freshly cloned repo, replace x.y.z with real version:

```bash
cd /tmp
rm -rf ultrajson
git clone https://github.com/ultrajson/ultrajson
cd ultrajson
python3 scripts/build-manylinux.py x.y.z
twine upload dist/*
```

- [ ] Check installation:

```bash
pip3 uninstall -y ujson && pip3 install -U ujson
python3 -c "import ujson; print(ujson.__version__)"
```
