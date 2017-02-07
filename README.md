# UltraJSON


[Original project](https://github.com/esnme/ultrajson).

In that fork are only debian package configs.

## How to build .deb & upload it to the debian repository

* `apt-get install python-all python-all-dev debhelper`
* `pip install --user stdeb`
* configure your `~/.dput.cf` (ex. `hh-apps` as the main configuration)
* configure your gpg keys
* `git clone git@github.com:hhru/ultrajson.git`
* set postfix for debian version in `setup.cfg`
* `cd ultrajson`
* `python setup.py --command-packages=stdeb.command bdist_deb`
* `cd deb_dist`
* `debsign ultrajson_X.Y.Z-hhN_amd64.changes`
* `dput hh-apps ultrajson_X.Y.Z-hhN_amd64.changes`
