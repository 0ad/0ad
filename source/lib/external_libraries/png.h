// <png.h> includes <zlib.h>, which requires some fixes by our header.
#include "lib/external_libraries/zlib.h"

#include <png.h>

// automatically link against the required library
#if MSC_VERSION
# ifdef NDEBUG
#  pragma comment(lib, "libpng13.lib")
# else
#  pragma comment(lib, "libpng13d.lib")
# endif	// NDEBUG
#endif	// MSC_VERSION
