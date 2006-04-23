// Precompiled headers:

#ifdef _WIN32
# define HAVE_PCH
#endif

#ifdef _MSC_VER
# define _SCL_SECURE_NO_DEPRECATE // shut up, std::copy isn't deprecated
#endif

#ifdef HAVE_PCH

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <vector>
#include <string>
#include <set>
#include <stack>
#include <map>

#endif // HAVE_PCH
