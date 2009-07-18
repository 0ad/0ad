This folder contains SpiderMonkey version 1.6, downloaded from here:
http://ftp.mozilla.org/pub/mozilla.org/js/js-1.60.tar.gz

All the spidermonkey files are in src/js/, the files in src/ are scripts made
for pyrogenesis, installing the library into the path searched by our build
scripts.

There are a few minor local changes in js/src/config.mk to set JS_LIBDIR (it
would be a good idea to have a formal reference to all the changes though).
