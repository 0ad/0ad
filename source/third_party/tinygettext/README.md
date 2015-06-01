tinygettext
===========

tinygettext is a minimal replacement for gettext written in C++. It
can read `.po` files directly and doesn't need `.mo` files generated
from `.po`. It also can read the `.po` files from arbitary locations,
so it's better suited for non-Unix systems and situations in which one
wants to store or distribute `.po` files seperately from the software
itself. It is licensed under
[zlib license](http://en.wikipedia.org/wiki/Zlib_License).

The latest version can be found at:

* https://github.com/tinygettext/tinygettext


Detecting the locale setting
----------------------------

Different operating systems store the default locale in different
places; a portable way to find it is provided by FindLocale:

* http://icculus.org/~aspirin/findlocale/
