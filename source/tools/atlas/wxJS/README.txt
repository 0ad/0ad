Based on wxJavaScript 0.9.6, with some modifications:

Added #include "precompiled.h" to every .cpp file
Changed #include <jsapi.h> into #include <js/jsapi.h>

Renamed io/{init,constant}.cpp to io/io_{init,constant}.cpp (to prevent naming conflicts when we compile everything together)
Renamed ext/main.cpp to ext/ext_main.cpp
Removed wxGlobalMap from io/io_constant.cpp
Removed everything except wxjs::ext:: definitions from ext/main.cpp

Fixed warnings from common/apiwrap.h

Fixed bugs:
http://sourceforge.net/tracker/index.php?func=detail&aid=1730754&group_id=50913&atid=461416
http://sourceforge.net/tracker/index.php?func=detail&aid=1730960&group_id=50913&atid=461416
