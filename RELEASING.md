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

- [ ] Check the tagged GitHub Actions builds have deployed
      [source](https://github.com/ultrajson/ultrajson/actions?query=workflow%3ADeploy)
      and
      [wheels](https://github.com/ultrajson/ultrajson/actions?query=workflow%3A%22Deploy+wheels%22)
      to
      [PyPI](https://pypi.org/project/ujson/#history)

- [ ] Check installation:

```bash
pip3 uninstall -y ujson && pip3 install -U ujson
python3 -c "import ujson; print(ujson.__version__)"
```
