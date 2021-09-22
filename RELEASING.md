# Release Checklist

- [ ] Get `main` to the appropriate code release state.
      [GitHub Actions](https://github.com/ultrajson/ultrajson/actions) and
      [Travis CI](https://travis-ci.com/ultrajson/ultrajson) should be running
      cleanly for all merges to `main`.
      [![GitHub Actions status](https://github.com/ultrajson/ultrajson/workflows/Test/badge.svg)](https://github.com/ultrajson/ultrajson/actions)
      [![Build Status](https://app.travis-ci.com/ultrajson/ultrajson.svg?branch=main)](https://app.travis-ci.com/ultrajson/ultrajson)

- [ ] Edit release draft, adjust text if needed: https://github.com/ultrajson/ultrajson/releases

- [ ] Check next tag is correct, amend if needed

- [ ] Publish release

- [ ] Check the tagged GitHub Actions builds have deployed
      [source and wheels](https://github.com/ultrajson/ultrajson/actions?query=workflow%3ADeploy)
      to
      [PyPI](https://pypi.org/project/ujson/#history)

- [ ] Check installation:

```bash
pip3 uninstall -y ujson && pip3 install -U ujson
python3 -c "import ujson; print(ujson.__version__)"
```
